#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <jni.h>
#include <android/log.h>
#include <cstdlib>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "NativeAudio", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "NativeAudio", __VA_ARGS__)

// Engine and output mix
SLObjectItf engineObject = nullptr;
SLEngineItf engineEngine = nullptr;
SLObjectItf outputMixObject = nullptr;

// Recorder
SLObjectItf recorderObject = nullptr;
SLRecordItf recorderRecord = nullptr;
SLAndroidSimpleBufferQueueItf recorderBufferQueue = nullptr;
short recorderBuffer[16000];

// Player
SLObjectItf playerObject = nullptr;
SLPlayItf playerPlay = nullptr;
SLAndroidSimpleBufferQueueItf playerBufferQueue = nullptr;
short playerBuffer[16000];

// Initialize the audio engine
extern "C"
JNIEXPORT void JNICALL
Java_com_example_opensles_MainActivity_initAudioEngine(JNIEnv *env, jobject thiz) {
    SLresult result;

    // Create engine
    result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) return;

    // Realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) return;

    // Get the engine interface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (result != SL_RESULT_SUCCESS) return;

    // Create output mix
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) return;

    // Realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) return;

    LOGI("Audio engine initialized");
}

// Recorder callback
void recorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    LOGI("Recorder callback triggered");
    // Re-enqueue the buffer for continuous recording
    (*bq)->Enqueue(bq, recorderBuffer, sizeof(recorderBuffer));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_opensles_MainActivity_startRecording(JNIEnv *env, jobject thiz, jint inputSource) {
    SLresult result;

    // Configure audio source with the specified input source
    SLDataLocator_IODevice locDevice = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, nullptr};
    SLDataSource audioSrc = {&locDevice, nullptr};

    // Configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue locBufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
    SLDataFormat_PCM formatPcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_16,
                                  SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                  SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&locBufferQueue, &formatPcm};

    // Create audio recorder
    const SLInterfaceID ids[2] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION};
    const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audioSrc, &audioSnk, 2, ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Recorder creation failed: %d", result);
        return;
    }

    // Get the configuration interface
    SLAndroidConfigurationItf configItf;
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDCONFIGURATION, &configItf);
    if (result == SL_RESULT_SUCCESS) {
        // Set the recording preset
        result = (*configItf)->SetConfiguration(configItf, SL_ANDROID_KEY_RECORDING_PRESET, &inputSource, sizeof(inputSource));
        if (result == SL_RESULT_SUCCESS) {
            LOGI("Input source set to %d", inputSource);
        } else {
            LOGE("Failed to set input source: %d", result);
        }
    } else {
        LOGE("Failed to get configuration interface: %d", result);
    }

    // Realize the recorder
    result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Recorder realization failed: %d", result);
        return;
    }

    // Get the record interface
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Recorder interface retrieval failed: %d", result);
        return;
    }

    // Get the buffer queue interface
    result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recorderBufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Recorder buffer queue interface retrieval failed: %d", result);
        return;
    }

    // Register callback
    result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, recorderCallback, nullptr);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Recorder callback registration failed: %d", result);
        return;
    }

    // Enqueue buffer
    result = (*recorderBufferQueue)->Enqueue(recorderBufferQueue, recorderBuffer, sizeof(recorderBuffer));
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Recorder buffer enqueue failed: %d", result);
        return;
    }

    // Start recording
    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("Failed to start recording: %d", result);
        return;
    }

    LOGI("Recording started with input source %d", inputSource);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_opensles_MainActivity_stopRecording(JNIEnv *env, jobject thiz) {
    if (recorderRecord != nullptr) {
        (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
    }
    LOGI("Recording stopped");
}

// Playback callback
void playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    // Enqueue the next buffer
    (*bq)->Enqueue(bq, playerBuffer, sizeof(playerBuffer));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_opensles_MainActivity_startPlaying(JNIEnv *env, jobject thiz) {
    SLresult result;

    // Configure audio source
    SLDataLocator_AndroidSimpleBufferQueue locBufferQueue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};
    SLDataFormat_PCM formatPcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_16,
                                  SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                  SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&locBufferQueue, &formatPcm};

    // Configure audio sink
    SLDataLocator_OutputMix locOutputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&locOutputMix, nullptr};

    // Create audio player
    const SLInterfaceID ids[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &playerObject, &audioSrc,
                                                &audioSnk, 1, ids, req);
    if (result != SL_RESULT_SUCCESS) return;

    // Realize the player
    result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) return;

    // Get the play interface
    result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay);
    if (result != SL_RESULT_SUCCESS) return;

    // Get the buffer queue interface
    result = (*playerObject)->GetInterface(playerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                           &playerBufferQueue);
    if (result != SL_RESULT_SUCCESS) return;

    // Register callback
    result = (*playerBufferQueue)->RegisterCallback(playerBufferQueue, playerCallback, nullptr);
    if (result != SL_RESULT_SUCCESS) return;

    // Enqueue buffer
    result = (*playerBufferQueue)->Enqueue(playerBufferQueue, playerBuffer, sizeof(playerBuffer));
    if (result != SL_RESULT_SUCCESS) return;

    // Start playback
    result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) return;

    LOGI("Playback started");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_opensles_MainActivity_stopPlaying(JNIEnv *env, jobject thiz) {
    if (recorderRecord != nullptr) {
        (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_STOPPED);
        LOGI("Recording stopped");
    }
}
