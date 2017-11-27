#include <memory>
#include <node.h>
#include <nan.h>
#include "leveldb.h"

// Batch

class BatchWorker : public Nan::AsyncWorker {
public:
  BatchWorker (LevelDB *obj, Nan::Callback *callback, leveldb::WriteBatch* batch);

  virtual ~BatchWorker ();
  virtual void Execute ();

private:
  LevelDB* obj;
  leveldb::WriteBatch* batch;
  leveldb::WriteOptions* options;
};

BatchWorker::BatchWorker (LevelDB *obj, Nan::Callback *callback, leveldb::WriteBatch* batch)
  : Nan::AsyncWorker(callback)
  , obj(obj)
  , batch(batch)
{
  options = new leveldb::WriteOptions();
  options->sync = false;
};

BatchWorker::~BatchWorker () {
  delete batch;
  delete options;
}

void BatchWorker::Execute () {
  leveldb::Status status = obj->BatchCall(options, batch);
  if (!status.ok()) SetErrorMessage(status.ToString().c_str());
}

// GetSeq

class GetSeqWorker : public Nan::AsyncWorker {
public:
  GetSeqWorker (LevelDB *obj, v8::Local<v8::Object> options, Nan::Callback *callback);

  virtual ~GetSeqWorker ();
  virtual void Execute ();
  virtual void HandleOKCallback ();

private:
  LevelDB* obj;
  leveldb::Slice* start;
  std::string end;
  std::vector<std::pair<std::string, std::string> > result;
};

GetSeqWorker::GetSeqWorker (LevelDB *obj, v8::Local<v8::Object> options, Nan::Callback *callback)
  : Nan::AsyncWorker(callback)
  , obj(obj)
{
// fix: save data buffer?
  v8::Local<v8::Object> gtBuffer = options->Get(Nan::New("gt").ToLocalChecked())->ToObject();
  start = new leveldb::Slice(node::Buffer::Data(gtBuffer), node::Buffer::Length(gtBuffer));
  v8::Local<v8::Object> ltBuffer = options->Get(Nan::New("lt").ToLocalChecked())->ToObject();
  end = std::string(node::Buffer::Data(ltBuffer), node::Buffer::Length(ltBuffer));
};

GetSeqWorker::~GetSeqWorker () {
  delete start;
}

void GetSeqWorker::Execute () {
  leveldb::ReadOptions* options = new leveldb::ReadOptions();
  options->fill_cache = true;
  options->snapshot = obj->db->GetSnapshot();

  leveldb::Iterator* iter = obj->db->NewIterator(*options);
  for (iter->Seek(*start); iter->Valid() && iter->key().ToString() < end; iter->Next()) {
    result.push_back(std::make_pair(iter->key().ToString(), iter->value().ToString()));
  } 

  delete iter;
  obj->db->ReleaseSnapshot(options->snapshot);
  delete options;


  // if (!status.ok()) SetErrorMessage(status.ToString().c_str());
}

void GetSeqWorker::HandleOKCallback () {
  Nan::HandleScope scope;
  v8::Local<v8::Array> returnArray = Nan::New<v8::Array>(result.size());
  for (size_t idx = 0; idx < result.size(); ++idx) {
    auto row = result[idx];
    v8::Local<v8::Array> item = Nan::New<v8::Array>(2);
    Nan::Set(item, 0, Nan::CopyBuffer((char*) row.first.data(), row.first.size()).ToLocalChecked());
    Nan::Set(item, 1, Nan::CopyBuffer((char*) row.second.data(), row.second.size()).ToLocalChecked());
    Nan::Set(returnArray, idx, item);
  }

  v8::Local<v8::Value> argv[] = { Nan::Null(), returnArray };
  callback->Call(2, argv);
}

// LevelDB

v8::Local<v8::Function> LevelDB::Init () {
  Nan::EscapableHandleScope scope;

  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(LevelDB::New);
  tpl->SetClassName(Nan::New("LevelDB").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "open", LevelDB::Open);
  Nan::SetPrototypeMethod(tpl, "batch", LevelDB::Batch);
  // Nan::SetPrototypeMethod(tpl, "get", LevelDB::Get);
  Nan::SetPrototypeMethod(tpl, "getSeq", LevelDB::GetSeq);
  Nan::SetPrototypeMethod(tpl, "close", LevelDB::Close);

  return scope.Escape(Nan::GetFunction(tpl).ToLocalChecked());
}

LevelDB::LevelDB (const v8::Local<v8::Value>& from)
  : db(NULL)
  , location(new Nan::Utf8String(from))
  , blockCache(NULL)
  , filterPolicy(NULL) {};

LevelDB::~LevelDB () {
  delete location;
  delete db;
  delete blockCache;
  delete filterPolicy;
};

NAN_METHOD(LevelDB::New) {
  LevelDB* obj = new LevelDB(info[0]);
  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(LevelDB::Open) {
  LevelDB* obj = Nan::ObjectWrap::Unwrap<LevelDB>(info.This());

  std::unique_ptr<leveldb::Options> options(new leveldb::Options());
  options->create_if_missing = true;
  options->error_if_exists = true;
  options->compression = leveldb::kSnappyCompression;
  options->write_buffer_size = 4 * 1024 * 1024;
  options->block_size = 4096;
  options->max_open_files = 1000;
  options->block_restart_interval = 16;
  options->max_file_size = 2 * 1024 * 1024;

  options->block_cache = obj->blockCache = leveldb::NewLRUCache(8 * 1024 * 1024);
  options->filter_policy = obj->filterPolicy = leveldb::NewBloomFilterPolicy(10);

  leveldb::Status status = leveldb::DB::Open(*options, **obj->location, &obj->db);
  if (!status.ok()) return Nan::ThrowError(status.ToString().c_str());
}

NAN_METHOD(LevelDB::Batch) {
  v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(info[0]);
  leveldb::WriteBatch* batch = new leveldb::WriteBatch();
  for (unsigned int i = 0; i < array->Length(); i++) {
    if (!array->Get(i)->IsObject())
      continue;

    v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(array->Get(i));
    v8::Local<v8::Object> keyBuffer = obj->Get(Nan::New("key").ToLocalChecked())->ToObject();
    v8::Local<v8::Value> type = obj->Get(Nan::New("type").ToLocalChecked());

    if (type->StrictEquals(Nan::New("del").ToLocalChecked())) {
      /*
      LD_STRING_OR_BUFFER_TO_SLICE(key, keyBuffer, key)

      batch->Delete(key);
      if (!hasData)
        hasData = true;

      DisposeStringOrBufferFromSlice(keyBuffer, key);
      */
    } else if (type->StrictEquals(Nan::New("put").ToLocalChecked())) {
      v8::Local<v8::Object> valueBuffer = obj->Get(Nan::New("value").ToLocalChecked())->ToObject();

// fix: save data buffer?
      leveldb::Slice key(node::Buffer::Data(keyBuffer), node::Buffer::Length(keyBuffer));
      leveldb::Slice value(node::Buffer::Data(valueBuffer), node::Buffer::Length(valueBuffer));

      batch->Put(key, value);
    }
  }

  v8::Local<v8::Function> callback = info[1].As<v8::Function>();
  if (array->Length() > 0) {
    LevelDB* obj = Nan::ObjectWrap::Unwrap<LevelDB>(info.This());
    BatchWorker* worker = new BatchWorker(obj, new Nan::Callback(callback), batch);
    Nan::AsyncQueueWorker(worker);
  } else {
    Nan::MakeCallback(Nan::GetCurrentContext()->Global(), callback, 0, NULL);
  }
}

NAN_METHOD(LevelDB::GetSeq) {
  LevelDB* obj = Nan::ObjectWrap::Unwrap<LevelDB>(info.This());
  v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[0]);
  v8::Local<v8::Function> callback = info[1].As<v8::Function>();
  GetSeqWorker* worker = new GetSeqWorker(obj, options, new Nan::Callback(callback));
  Nan::AsyncQueueWorker(worker);
}

NAN_METHOD(LevelDB::Close) {}
