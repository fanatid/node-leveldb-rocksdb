#ifndef LEVELDB_H
#define LEVELDB_H

#include <node.h>
#include <nan.h>
#include <leveldb/db.h>
#include <leveldb/cache.h>
#include <leveldb/filter_policy.h>
#include <leveldb/write_batch.h>

class LevelDB : public Nan::ObjectWrap {
public:
  static v8::Local<v8::Function> Init ();

  leveldb::Status BatchCall (leveldb::WriteOptions* options, leveldb::WriteBatch* batch) {
    return db->Write(*options, batch);
  }

  leveldb::DB* db;
private:
  Nan::Utf8String* location;
  leveldb::Cache* blockCache;
  const leveldb::FilterPolicy* filterPolicy;

  LevelDB (const v8::Local<v8::Value>& from);
  ~LevelDB ();

  static NAN_METHOD(New);
  static NAN_METHOD(Open);
  static NAN_METHOD(Batch);
  static NAN_METHOD(GetSeq);
  static NAN_METHOD(Close);
};

#endif
