#include "BloomFilter.hpp"

#include <node.h>
#include <node_buffer.h>
#include <node_object_wrap.h>
#include <v8.h>

using namespace v8;

class NodeBloomFilter : public node::ObjectWrap {
public:
  static void Init(Handle<Object> exports);
  static Persistent<Function> constructor;
  BloomFilter bloomFilter;

private:
  explicit NodeBloomFilter(size_t nBuckets, uint8_t nHashes)
  : bloomFilter(nBuckets, nHashes) {}

  static void New(const FunctionCallbackInfo<Value>& args);
  static void Add(const FunctionCallbackInfo<Value>& args);
  static void Test(const FunctionCallbackInfo<Value>& args);
  static void Serialize(const FunctionCallbackInfo<Value>& args);
};

Persistent<Function> NodeBloomFilter::constructor;

void NodeBloomFilter::Init(Handle<Object> exports) {
  Isolate* isolate = Isolate::GetCurrent();

  // Prepare constructor template
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "BloomFilter"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(tpl, "add", Add);
  NODE_SET_PROTOTYPE_METHOD(tpl, "test", Test);
  NODE_SET_PROTOTYPE_METHOD(tpl, "serialize", Serialize);

  constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "BloomFilter"), tpl->GetFunction());
}

void NodeBloomFilter::New(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  if (args.IsConstructCall()) {
    // Invoked as constructor: `new MyObject(...)`
    uint32_t nBuckets = args[0]->IsUndefined() ? 0 : static_cast<uint32_t>(args[0]->Int32Value());
    uint8_t nHashes = args[1]->IsUndefined() ? 0 : static_cast<uint8_t>(args[1]->Int32Value());
    NodeBloomFilter* obj = new NodeBloomFilter(nBuckets, nHashes);
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    // Invoked as plain function `MyObject(...)`, turn into construct call
    const int argc = 2;
    Local<Value> argv[argc] = { args[0], args[1] };
    Local<Function> cons = Local<Function>::New(isolate, constructor);
    MaybeLocal<Object> maybeInstance = cons->NewInstance(isolate->GetCurrentContext(), argc, argv);
    args.GetReturnValue().Set(maybeInstance.ToLocalChecked());
  }
}

void NodeBloomFilter::Add(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  NodeBloomFilter* obj = ObjectWrap::Unwrap<NodeBloomFilter>(args.Holder());

  Local<Value> arg = args[0]; // Buffer or String
  if (node::Buffer::HasInstance(arg)) {
    // It's a buffer. Go char-by-char
    obj->bloomFilter.add(node::Buffer::Data(arg), node::Buffer::Length(arg));
  } else {
    // We can convert it to utf-8. On failure, it's just an empty String.
    String::Utf8Value argString(arg);
    obj->bloomFilter.add(*argString, argString.length());
  }
}

void NodeBloomFilter::Test(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  Local<Context> context = isolate->GetCurrentContext();
  HandleScope scope(isolate);
  NodeBloomFilter* obj = ObjectWrap::Unwrap<NodeBloomFilter>(args.Holder());

  bool ret;

  Local<Value> arg = args[0]; // Buffer or String
  if (node::Buffer::HasInstance(arg)) {
    // It's a buffer. Go char-by-char
    const char* data(node::Buffer::Data(arg));
    const size_t len(node::Buffer::Length(arg));

    size_t begin = 0;
    size_t end = len;
    if (args[1]->IsUint32()) {
      begin = args[1]->ToUint32(context).ToLocalChecked()->Value();
    }
    if (args[2]->IsUint32()) {
      end = args[2]->ToUint32(context).ToLocalChecked()->Value();
    }

    // clamp values
    if (begin > len) begin = len;
    if (end > len) end = len;
    if (end < begin) end = begin;

    ret = obj->bloomFilter.test(data + begin, end - begin);
  } else {
    // We can convert it to utf-8. On failure, it's just an empty String.
    String::Utf8Value argString(arg);
    ret = obj->bloomFilter.test(*argString, argString.length());
  }

  args.GetReturnValue().Set(ret);
}

void NodeBloomFilter::Serialize(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  NodeBloomFilter* obj = ObjectWrap::Unwrap<NodeBloomFilter>(args.Holder());

  const std::string data = obj->bloomFilter.serialize();
  MaybeLocal<Object> buffer = node::Buffer::Copy(isolate, data.c_str(), data.length());

  args.GetReturnValue().Set(buffer.ToLocalChecked());
}

void Unserialize(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  Local<Context> context = isolate->GetCurrentContext();
  HandleScope scope(isolate);

  Local<Value> arg = args[0];
  if (!node::Buffer::HasInstance(arg)) {
    isolate->ThrowException(Exception::TypeError(
      String::NewFromUtf8(isolate, "Argument must be a Buffer")));
    return;
  }
  if (node::Buffer::Length(arg) < 5) {
    isolate->ThrowException(Exception::TypeError(
      String::NewFromUtf8(isolate, "Buffer is too short to represent a BloomFilter")));
    return;
  }

  const char* data(node::Buffer::Data(arg));
  const size_t len(node::Buffer::Length(arg));

  // Read nBuckets: four bytes
  uint32_t nBucketsN = 0;
  nBucketsN |= static_cast<uint32_t>(data[0]) << 24;
  nBucketsN |= static_cast<uint32_t>(data[1]) << 16;
  nBucketsN |= static_cast<uint32_t>(data[2]) << 8;
  nBucketsN |= static_cast<uint32_t>(data[3]);
  const uint32_t nBuckets = ntohl(nBucketsN);

  // Read nHashes: one byte
  const uint8_t nHashes = static_cast<uint8_t>(data[4]);

  // Okay, now we can build the instance
  Local<Value> argv[2] = {
    Integer::NewFromUnsigned(isolate, nBuckets),
    Integer::NewFromUnsigned(isolate, nHashes)
  };
  Local<Function> cons = Local<Function>::New(isolate, NodeBloomFilter::constructor);
  MaybeLocal<Object> ret = cons->NewInstance(context, 2, argv);
  NodeBloomFilter* obj = node::ObjectWrap::Unwrap<NodeBloomFilter>(ret.ToLocalChecked());
  obj->bloomFilter.setBuckets(&data[5], len - 5);

  args.GetReturnValue().Set(ret.ToLocalChecked());
}

void init(Handle<Object> exports) {
  NodeBloomFilter::Init(exports);

  NODE_SET_METHOD(exports, "unserialize", Unserialize);
}

NODE_MODULE(binding, init);
