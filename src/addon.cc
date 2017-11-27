#include <node.h>
#include <nan.h>
#include "leveldb.h"
// #include "rocks.h"

void Init(v8::Local<v8::Object> exports) {
  Nan::HandleScope scope;
  Nan::Set(exports, Nan::New("LevelDB").ToLocalChecked(), LevelDB::Init());
}

NODE_MODULE(addon, Init)
