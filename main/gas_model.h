// gas_model.h
//
// Unprotected TinyML baseline for the thesis.
// Ported from the R&D project's real "ethanol_detect" ONNX model
// (28 engineered features -> 32 -> 16 -> 2, ReLU/ReLU/Softmax).
//
// This header is the model being protected / attacked. On the
// unprotected build it sits in flash in the open; once Secure Boot V2 +
// Flash Encryption are enabled, the same file is compiled into a
// signed, encrypted image.

#pragma once
#include <stdint.h>

#define GAS_WINDOW_STEPS 10   // one heater-profile cycle
#define GAS_RAW_FIELDS   4    // resistance, temperature, duration, heaterProfileID
#define GAS_N_FEATURES   28   // 10 centered-log + 9 deltas + 1 mean + 8 one-hot
#define GAS_N_CLASSES    2    // 0 = ethanol, 1 = water

// window[step][field], field order: resistance, temperature, duration, heaterProfileID
// (same layout as the "window" ONNX graph input)
typedef float gas_window_t[GAS_WINDOW_STEPS][GAS_RAW_FIELDS];

// Runs the full feature pipeline + MLP. Writes GAS_N_CLASSES
// probabilities (softmax) into out_probs. Returns the predicted
// class index (argmax).
int gas_model_predict(const gas_window_t window, float out_probs[GAS_N_CLASSES]);
