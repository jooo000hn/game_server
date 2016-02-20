/*
 * V8_Manager.cpp
 *
 *  Created on: 2016年1月22日
 *      Author: zhangyalei
 */

#include "V8_Manager.h"
#include "V8_Wrap.h"
#include "Buffer_Wrap.h"

V8_Manager::V8_Manager(void):platform_(nullptr), isolate_(nullptr) { }

V8_Manager::~V8_Manager(void) { }

V8_Manager *V8_Manager::instance_;

V8_Manager *V8_Manager::instance(void) {
	if (instance_ == 0)
		instance_ = new V8_Manager;
	return instance_;
}

void V8_Manager::run_handler(void) {
	init();
}

int V8_Manager::init(void) {
	//初始化V8
	V8::InitializeICU();
	V8::InitializeExternalStartupData("");
	platform_ = platform::CreateDefaultPlatform();
	V8::InitializePlatform(platform_);
	V8::Initialize();

	ArrayBufferAllocator allocator;
	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = &allocator;
	isolate_ = Isolate::New(create_params);
	//进入v8的Isolate内部，才能使用V8引擎
	Isolate::Scope isolate_scope(isolate_);
	//创建V8执行环境
	HandleScope handle_scope(isolate_);
	Local<Context> context = create_v8_context(isolate_);
	context_.Reset(isolate_, context);
	//进入V8执行环境内部
	Context::Scope context_scope(context);

	run_script(isolate_, "js/server.js");
	return 0;
}

int V8_Manager::fini(void) {
	//释放V8资源
	isolate_->LowMemoryNotification();
	context_.Reset();
	load_data_.Reset();
	V8::Dispose();
	V8::ShutdownPlatform();
	delete platform_;
	return 0;
}

int V8_Manager::js_load_player_data(Block_Buffer &buf) {
	//执行V8代码前，必须先进入V8的Isolate，初始化V8运行环境
	Isolate::Scope isolate_scope(isolate_);
	HandleScope handle_scope(isolate_);
	Local<Context> context = Local<Context>::New(isolate_, context_);
	Context::Scope context_scope(context);

	//获取js函数
	Local<String> func_name = String::NewFromUtf8(isolate_, "load_data", NewStringType::kNormal).ToLocalChecked();
	Local<Value> func_value;
	if (!context->Global()->Get(context, func_name).ToLocal(&func_value) || !func_value->IsFunction()) {
	    return -1;
	 }
	//转换成js函数对象
	Local<Function> js_func = Local<Function>::Cast(func_value);

	// Invoke the process function, giving the global object as 'this'
	TryCatch try_catch(isolate_);
	Local<Object> buf_obj = wrap_buffer(isolate_, &buf);
	const int argc = 1;
	Local<Value> argv[argc] = {buf_obj};
	Local<Value> result;
	if (!js_func->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
		String::Utf8Value error(try_catch.Exception());
	  printf("js_load_player_data error, : %s", *error);
		return -1;
	}
	return 0;
}
