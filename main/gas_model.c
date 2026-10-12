// gas_model.c
//
// Faithful C port of ethanol_detect.onnx's feature pipeline + MLP.
// Field indices into gas_window_t: 0=resistance, 1=temperature, 2=duration, 3=heaterProfileID.

#include <math.h>
#include <string.h>
#include "gas_model.h"
#include "model_weights.h"

static inline float relu(float x) { return x > 0.0f ? x : 0.0f; }

int gas_model_predict(const gas_window_t window, float out_probs[GAS_N_CLASSES]) {
    // --- 1. log(resistance), clipped to >= 1.0 before the log ---
    float log_r[GAS_WINDOW_STEPS];
    for (int t = 0; t < GAS_WINDOW_STEPS; t++) {
        float r = window[t][0];
        if (r < 1.0f) r = 1.0f;
        log_r[t] = logf(r);
    }

    // --- 2. mean(log_r) over the 10 steps ---
    float mean_log_r = 0.0f;
    for (int t = 0; t < GAS_WINDOW_STEPS; t++) mean_log_r += log_r[t];
    mean_log_r /= GAS_WINDOW_STEPS;

    // --- 3. centered log-resistance curve: log_r - mean_log_r  (10 features) ---
    float centered[GAS_WINDOW_STEPS];
    for (int t = 0; t < GAS_WINDOW_STEPS; t++) centered[t] = log_r[t] - mean_log_r;

    // --- 4. step-to-step deltas: log_r[t+1] - log_r[t]  (9 features) ---
    float deltas[GAS_WINDOW_STEPS - 1];
    for (int t = 0; t < GAS_WINDOW_STEPS - 1; t++) deltas[t] = log_r[t + 1] - log_r[t];

    // --- 5. one-hot of heaterProfileID (rounded), profiles 1..8  (8 features) ---
    float heater_id = window[0][3];
    long rounded = lroundf(heater_id);
    float one_hot[8];
    for (int i = 0; i < 8; i++) one_hot[i] = (rounded == (i + 1)) ? 1.0f : 0.0f;

    // --- 6. concat: [centered(10), deltas(9), mean_log_r(1), one_hot(8)] = 28 ---
    float feat[GAS_N_FEATURES];
    int k = 0;
    for (int t = 0; t < GAS_WINDOW_STEPS; t++)     feat[k++] = centered[t];
    for (int t = 0; t < GAS_WINDOW_STEPS - 1; t++) feat[k++] = deltas[t];
    feat[k++] = mean_log_r;
    for (int i = 0; i < 8; i++)                    feat[k++] = one_hot[i];
    // k == GAS_N_FEATURES == 28 here

    // --- 7. normalize: (feat - mean) / std ---
    float x0[GAS_N_FEATURES];
    for (int i = 0; i < GAS_N_FEATURES; i++) {
        x0[i] = (feat[i] - MEAN_28[i]) / STD_28[i];
    }

    // --- 8. Linear(28->32) + ReLU ---
    float h0[32];
    for (int o = 0; o < 32; o++) {
        float acc = B0_32[o];
        for (int i = 0; i < GAS_N_FEATURES; i++) acc += W0_32x28[o][i] * x0[i];
        h0[o] = relu(acc);
    }

    // --- 9. Linear(32->16) + ReLU ---
    float h1[16];
    for (int o = 0; o < 16; o++) {
        float acc = B1_16[o];
        for (int i = 0; i < 32; i++) acc += W1_16x32[o][i] * h0[i];
        h1[o] = relu(acc);
    }

    // --- 10. Linear(16->2) ---
    float logits[GAS_N_CLASSES];
    for (int o = 0; o < GAS_N_CLASSES; o++) {
        float acc = B2_2[o];
        for (int i = 0; i < 16; i++) acc += W2_2x16[o][i] * h1[i];
        logits[o] = acc;
    }

    // --- 11. softmax ---
    float maxv = logits[0] > logits[1] ? logits[0] : logits[1];
    float e0 = expf(logits[0] - maxv);
    float e1 = expf(logits[1] - maxv);
    float sum = e0 + e1;
    out_probs[0] = e0 / sum;
    out_probs[1] = e1 / sum;

    return (out_probs[0] > out_probs[1]) ? 0 : 1; // 0=ethanol, 1=water
}
