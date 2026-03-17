#pragma once

#include <jni.h>

bool IsDialogLoginValidated();
void RegisterDialogContext(JNIEnv* env, jobject context);
const char* SubmitJavaLogin(JNIEnv* env, jobject context, const char* email, const char* password);
const char* GetJavaLoginSuccessSummary();
void QueueLibLoadDialog(const char* title, const char* message);
void ShowQueuedLibLoadDialog(JNIEnv* env, jobject context);
void ShowQueuedLibLoadDialogWithRegisteredContext(JNIEnv* env);
void QueueLoginSuccessHints(const char* displayName, int remainingDays);
