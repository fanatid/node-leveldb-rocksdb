#ifndef ROCKS_H
#define ROCKS_H

#include <nan.h>

class RocksDB : public Nan::ObjectWrap {
public:
  static v8::Local<v8::Function> Init () {
    Nan::EscapableHandleScope scope;

    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("RocksDB").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Nan::SetPrototypeMethod(tpl, "batch", Batch);
    // Nan::SetPrototypeMethod(tpl, "multiGet", MultiGet);
    // Nan::SetPrototypeMethod(tpl, "iterator", Iterator);
    Nan::SetPrototypeMethod(tpl, "close", Close);

    return scope.Escape(Nan::GetFunction(tpl).ToLocalChecked());
  }

private:
  static NAN_METHOD(New) {
    // v8::Local<v8::String> location = info[0].As<v8::String>();
    // RocksDB* obj = new RocksDB();
    // obj->Wrap(info.This());
    // info.GetReturnValue().Set(info.This());
  }

  static NAN_METHOD(Close) {}
};

#endif
