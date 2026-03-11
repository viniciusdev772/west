#pragma once

#include <jni.h>

bool IsDialogLoginValidated();
void RegisterDialogContext(JNIEnv* env, jobject context);
void QueueLibLoadDialog(const char* title, const char* message);
void ShowQueuedLibLoadDialog(JNIEnv* env, jobject context);
