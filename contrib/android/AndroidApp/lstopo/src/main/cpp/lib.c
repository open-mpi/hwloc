//
// Created by Hoyet on 11/04/2019.
//
#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <android/log.h>
#include <syslog.h>
#include "../../../../../../../hwloc/misc.c"
#include "../../../../../../../hwloc/bitmap.c"
#include "../../../../../../../hwloc/distances.c"
#include "../../../../../../../hwloc/topology.c"
#include "../../../../../../../hwloc/traversal.c"
#include "../../../../../../../hwloc/bind.c"
#include "../../../../../../../hwloc/components.c"
#include "../../../../../../../hwloc/pci-common.c"
#include "../../../../../../../hwloc/topology-linux.c"
#include "../../../../../../../hwloc/shmem.c"
#include "../../../../../../../hwloc/memattrs.c"
#include "../../../../../../../hwloc/cpukinds.c"
#include "../../../../../../../hwloc/topology-xml.c"
#include "../../../../../../../hwloc/topology-xml-nolibxml.c"
#include "../../../../../../../hwloc/base64.c"
#include "../../../../../../../hwloc/topology-hardwired.c"
#include "../../../../../../../hwloc/topology-noos.c"
#include "../../../../../../../hwloc/topology-synthetic.c"
#include "../../../../../../../utils/hwloc/common-ps.c"
#include "../../../../../../../utils/lstopo/lstopo.c"
#include "../../../../../../../utils/lstopo/lstopo-text.c"
#include "../../../../../../../utils/lstopo/lstopo-ascii.c"
#include "../../../../../../../utils/lstopo/lstopo-draw.c"
#include "../../../../../../../utils/lstopo/lstopo-fig.c"
#include "../../../../../../../utils/lstopo/lstopo-svg.c"
#include "../../../../../../../utils/lstopo/lstopo-xml.c"
#include "../../../../../../../utils/lstopo/lstopo-android.c"
#include "../../../../../../../utils/lstopo/lstopo-tikz.c"
#include "../../../../../../../utils/lstopo/lstopo-shmem.c"

typedef struct {
    jclass lstopo_android;
    jmethodID box;
    jmethodID text;
    jmethodID line;
    jmethodID debug;
    jmethodID clearDebug;
} tools_methods;

typedef struct {
    jobject lstopo;
    int mode;
    JNIEnv *jni;
    tools_methods methods;
} JNItools;

JNItools tools;

JNIEXPORT int JNICALL Java_com_hwloc_lstopo_MainActivity_start(JNIEnv *env, jobject _this, jobject *object, int drawing_method, jstring outputFile, jobject options) {
    tools.mode = drawing_method;
    tools.lstopo = object;
    tools.jni = env;
    tools.methods.lstopo_android = 0;

    const char * c_outputFile = (*tools.jni)->GetStringUTFChars(tools.jni, outputFile, 0);
    int options_total_count = 0;
    char *argv[16];
    argv[options_total_count++] = "lstopo";

    jclass alCls = (*tools.jni)->FindClass(tools.jni, "java/util/ArrayList");
    jmethodID alGetId  = (*tools.jni)->GetMethodID(tools.jni, alCls, "get", "(I)Ljava/lang/Object;");
    jmethodID alSizeId = (*tools.jni)->GetMethodID(tools.jni,alCls, "size", "()I");
    int options_args_count = (int) ((*tools.jni)->CallIntMethod(tools.jni, options, alSizeId));

    for (int i = 0; i < options_args_count; i++) {
        jobject str = (*tools.jni)->CallObjectMethod(tools.jni, options, alGetId, i);
        const char *rawString = (*tools.jni)->GetStringUTFChars(tools.jni, str, 0);
        argv[options_total_count++] = (char*) rawString;
    }

    if(drawing_method == 2) {
        argv[options_total_count++] = "-v";
        argv[options_total_count++] = "--of";
        argv[options_total_count++] = "console";
        argv[options_total_count++] = (char *) c_outputFile;
        return main(options_total_count, argv);

    } else if(drawing_method == 3) {
        argv[options_total_count++] = "--of";
        argv[options_total_count++] = "xml";
        argv[options_total_count++] = (char *) c_outputFile;
        return main(options_total_count, argv);

    } else {
        return main(options_total_count, argv);
    }
}

JNIEXPORT int JNICALL Java_com_hwloc_lstopo_MainActivity_startWithInput(JNIEnv *env, jobject _this, jobject *object, int drawing_method, jstring outputFile, jstring inputFile, jobjectArray options) {
    tools.mode = drawing_method;
    tools.lstopo = object;
    tools.jni = env;
    tools.methods.lstopo_android = 0;

    const char * c_outputFile = (*tools.jni)->GetStringUTFChars(tools.jni, outputFile, 0);
    const char * c_inputFile = (*tools.jni)->GetStringUTFChars(tools.jni, inputFile, 0);
    int options_total_count = 0;
    char *argv[16];
    argv[options_total_count++] = "lstopo";

    jclass alCls = (*tools.jni)->FindClass(tools.jni, "java/util/ArrayList");
    jmethodID alGetId  = (*tools.jni)->GetMethodID(tools.jni, alCls, "get", "(I)Ljava/lang/Object;");
    jmethodID alSizeId = (*tools.jni)->GetMethodID(tools.jni,alCls, "size", "()I");
    int options_args_count = (int) ((*tools.jni)->CallIntMethod(tools.jni, options, alSizeId));

    for (int i = 0; i < options_args_count; i++) {
        jobject str = (*tools.jni)->CallObjectMethod(tools.jni, options, alGetId, i);
        const char *rawString = (*tools.jni)->GetStringUTFChars(tools.jni, str, 0);
        argv[options_total_count++] = (char*) rawString;
    }

    if(drawing_method == 2) {
        argv[options_total_count++] = "-v";
        argv[options_total_count++] = "-i";
        argv[options_total_count++] = (char *) c_inputFile;
        argv[options_total_count++] = "--of";
        argv[options_total_count++] = "console";
        argv[options_total_count++] = (char *) c_outputFile;
        return main(options_total_count, argv);

    } else if(drawing_method == 3) {
        argv[options_total_count++] = "-i";
        argv[options_total_count++] = (char *) c_inputFile;
        argv[options_total_count++] = "--of";
        argv[options_total_count++] = "xml";
        argv[options_total_count++] = (char *) c_outputFile;
        return main(options_total_count, argv);

    } else {
        argv[options_total_count++] = "-i";
        argv[options_total_count++] = (char *) c_inputFile;
        return main(options_total_count, argv);
    }
}

void JNIprepare(int height, int width, int fontsize){
    jmethodID prepare = (*tools.jni)->GetMethodID(tools.jni, tools.methods.lstopo_android, "setScale", "(III)V");
    (*tools.jni)->CallVoidMethod(tools.jni, tools.lstopo, prepare, height, width, fontsize);
}

void JNIbox(int r, int g, int b, int x, int y, int width, int height, int gp_index, char *info) {
    size_t length = strlen(info);
    jbyteArray array = (*tools.jni)->NewByteArray(tools.jni, length);
    (*tools.jni)->SetByteArrayRegion(tools.jni, array, 0, length, (const jbyte *) info);
    jstring strEncode = (*tools.jni)->NewStringUTF(tools.jni, "UTF-8");
    jclass class = (*tools.jni)->FindClass(tools.jni, "java/lang/String");
    jmethodID ctor = (*tools.jni)->GetMethodID(tools.jni, class, "<init>", "([BLjava/lang/String;)V");
    jstring str = (jstring) (*tools.jni)->NewObject(tools.jni, class, ctor, array, strEncode);

    (*tools.jni)->CallVoidMethod(tools.jni, tools.lstopo, tools.methods.box, r, g, b, x, y, width, height, gp_index, str);
    (*tools.jni)->DeleteLocalRef(tools.jni, str);
    (*tools.jni)->DeleteLocalRef(tools.jni, array);
    (*tools.jni)->DeleteLocalRef(tools.jni, strEncode);
    (*tools.jni)->DeleteLocalRef(tools.jni, class);
}

void JNItext(char *text, int gp_index, int x, int y, int fontsize) {
    size_t length = strlen(text);
    jbyteArray array = (*tools.jni)->NewByteArray(tools.jni, length);
    (*tools.jni)->SetByteArrayRegion(tools.jni, array, 0, length, (const jbyte *) text);
    jstring strEncode = (*tools.jni)->NewStringUTF(tools.jni, "UTF-8");
    jclass class = (*tools.jni)->FindClass(tools.jni, "java/lang/String");
    jmethodID ctor = (*tools.jni)->GetMethodID(tools.jni, class, "<init>", "([BLjava/lang/String;)V");
    jstring str = (jstring) (*tools.jni)->NewObject(tools.jni, class, ctor, array, strEncode);

    (*tools.jni)->CallVoidMethod(tools.jni, tools.lstopo, tools.methods.text, str, x, y, fontsize, gp_index);
    (*tools.jni)->DeleteLocalRef(tools.jni, str);
    (*tools.jni)->DeleteLocalRef(tools.jni, array);
    (*tools.jni)->DeleteLocalRef(tools.jni, strEncode);
    (*tools.jni)->DeleteLocalRef(tools.jni, class);
}

void JNIline(unsigned x1, unsigned y1, unsigned x2, unsigned y2) {
    if (tools.methods.lstopo_android == 0) {
        jclass cls1 = (*tools.jni)->GetObjectClass(tools.jni, tools.lstopo);
        if (cls1 == 0)
            tools.methods.lstopo_android = (*tools.jni)->NewGlobalRef(tools.jni, cls1);
    }
    (*tools.jni)->CallVoidMethod(tools.jni, tools.lstopo, tools.methods.line, x1 , y1 , x2, y2);
}

void JNIDebug(char *text) {
    size_t length = strlen(text);
    jbyteArray array = (*tools.jni)->NewByteArray(tools.jni, length);
    (*tools.jni)->SetByteArrayRegion(tools.jni, array, 0, length, (const jbyte *) text);
    jstring strEncode = (*tools.jni)->NewStringUTF(tools.jni, "UTF-8");
    jclass class = (*tools.jni)->FindClass(tools.jni, "java/lang/String");
    jmethodID ctor = (*tools.jni)->GetMethodID(tools.jni, class, "<init>", "([BLjava/lang/String;)V");
    jstring str = (jstring) (*tools.jni)->NewObject(tools.jni, class, ctor, array, strEncode);

    (*tools.jni)->CallVoidMethod(tools.jni, tools.lstopo, tools.methods.debug, str);
    (*tools.jni)->DeleteLocalRef(tools.jni, str);
    (*tools.jni)->DeleteLocalRef(tools.jni, array);
    (*tools.jni)->DeleteLocalRef(tools.jni, strEncode);
    (*tools.jni)->DeleteLocalRef(tools.jni, class);
}

void setJNIEnv() {
    tools.methods.lstopo_android = (*tools.jni)->GetObjectClass(tools.jni, tools.lstopo);
    tools.methods.box = (*tools.jni)->GetMethodID(tools.jni, tools.methods.lstopo_android, "box", "(IIIIIIIILjava/lang/String;)V");
    tools.methods.text = (*tools.jni)->GetMethodID(tools.jni, tools.methods.lstopo_android, "text", "(Ljava/lang/String;IIII)V");
    tools.methods.line = (*tools.jni)->GetMethodID(tools.jni, tools.methods.lstopo_android, "line", "(IIII)V");
    tools.methods.debug = (*tools.jni)->GetMethodID(tools.jni, tools.methods.lstopo_android, "writeDebugFile", "(Ljava/lang/String;)V");
    tools.methods.clearDebug = (*tools.jni)->GetMethodID(tools.jni, tools.methods.lstopo_android, "clearDebugFile", "()V");
}
