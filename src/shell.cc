// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <include/v8.h>

#include <include/libplatform/libplatform.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <utility>
#include <utility>      // std::pair
#include <iostream>     // std::cout
#include<stdio.h>
#include <time.h> 
#include <stdio.h> 
#include <cstdio>

#ifdef WIN32
#include "Windows.h"
#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define BACKGROUND_BLUE      0x0010 // background color contains blue.
#define BACKGROUND_GREEN     0x0020 // background color contains green.
#define BACKGROUND_RED       0x0040 // background color contains red.
#define BACKGROUND_INTENSITY 0x0080 // background color is intensified.
#else
#include <unistd.h>
#endif

#define RUN_UTEST
#define USE_MEM_MONITOR
#define USE_CHECK_TIME
#define USE_NEW_IOSLATE

#ifdef USE_CHECK_TIME
char check_time[100] = "__enable_check_time__";
#else
char check_time[100] = { 0 };
#endif

int g_repetition_count = 10000;
unsigned int g_check_block_time_count = 0;
unsigned int g_check_all_time_count = 0;

#if 0
#define LOG_INFO printf
#else
void shell_no_printf(const char* format, ...) {}
#define LOG_INFO shell_no_printf
#endif

std::map<std::string, std::string> file_script_map_;

static int64_t Stoi64(const std::string &str) 
{
	int64_t v = 0;
#ifdef WIN32
	sscanf_s(str.c_str(), "%lld", &v);
#else
	sscanf(str.c_str(), "%ld", &v);
#endif
	return v;
}

/**
 * This sample program shows how to implement a simple javascript shell
 * based on V8.  This includes initializing V8 with command line options,
 * creating global functions, compiling and executing strings.
 *
 * For a more sophisticated shell, consider using the debug shell D8.
 */


v8::Local<v8::Context> CreateShellContext(v8::Isolate* isolate);
void RunShell(v8::Local<v8::Context> context, v8::Platform* platform);
void RunUtest(v8::Local<v8::Context> context, v8::Platform* platform);
int RunMain(v8::Isolate* isolate, v8::Platform* platform, int argc,
            char* argv[]);
bool ExecuteString(v8::Isolate* isolate, v8::Local<v8::String> source,
                   v8::Local<v8::Value> name, bool print_result,
                   bool report_exceptions);
void Print(const v8::FunctionCallbackInfo<v8::Value>& args);
void CheckTime(const v8::FunctionCallbackInfo<v8::Value>& args);
void Add(const v8::FunctionCallbackInfo<v8::Value>& args);
void Include(const v8::FunctionCallbackInfo<v8::Value>& args);
void Read(const v8::FunctionCallbackInfo<v8::Value>& args);
void Load(const v8::FunctionCallbackInfo<v8::Value>& args);
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);
void Version(const v8::FunctionCallbackInfo<v8::Value>& args);
v8::MaybeLocal<v8::String> ReadFile(v8::Isolate* isolate, const char* name);
void ReportException(v8::Isolate* isolate, v8::TryCatch* handler);

static const char* last_location;
static const char* last_message;
void StoringErrorCallback(const char* location, const char* message) {
	if (last_location == NULL) {
		last_location = location;
		last_message = message;
	}
	printf("location:%s, message:%s\n", location, message);
}


static bool run_shell;

int Repetition(int argc, char* argv[])
{
	v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(argv[0]);
	v8::Platform* platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
	v8::V8::Initialize();
	v8::V8::SetFlagsFromCommandLine(&argc, argv, true);

	time_t start_t = time(0);
	char start_tmp[64];
	strftime(start_tmp, sizeof(start_tmp), "%Y/%m/%d %X", localtime(&start_t));
	printf("Utest start time:%s\n", start_tmp);
	int result;
	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator =
		v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	/*create_params.constraints.set_code_range_size(512);
	create_params.constraints.set_max_executable_size(2 * 1024);
	create_params.constraints.set_max_old_space_size(2 * 1024);
	create_params.constraints.set_max_semi_space_size(512);*/
#ifndef USE_NEW_IOSLATE
	v8::Isolate* isolate = v8::Isolate::New(create_params);
	isolate->SetFatalErrorHandler(StoringErrorCallback);
#endif

	last_location = last_message = NULL;
	for (int i = 0; i < g_repetition_count; i++)
	{
#ifdef USE_NEW_IOSLATE
		v8::Isolate* isolate = v8::Isolate::New(create_params);
		isolate->SetFatalErrorHandler(StoringErrorCallback);
#endif
		{
			v8::Isolate::Scope isolate_scope(isolate);
			v8::HandleScope handle_scope(isolate);
			v8::Local<v8::Context> context = CreateShellContext(isolate);
			run_shell = (argc == 1);
			{

				if (context.IsEmpty()) {
					fprintf(stderr, "Error creating context\n");
					return 1;
				}
				v8::Context::Scope context_scope(context);
				result = RunMain(isolate, platform, argc, argv);
#ifdef RUN_UTEST
				RunUtest(context, platform);
#else
				if (run_shell) RunShell(context, platform);
#endif
			}
		}

#ifdef USE_NEW_IOSLATE
		isolate->Dispose();
#endif
	}
	
	delete create_params.array_buffer_allocator;
	time_t end_t = time(0);
	char end_tmp[64];
	strftime(end_tmp, sizeof(end_tmp), "%Y/%m/%d %X", localtime(&end_t));
	printf("Utest end time:%s\n", end_tmp);

	char buffer[10000];
	char* str = fgets(buffer, 10000, stdin);
#ifndef USE_NEW_IOSLATE
	isolate->Dispose();
#endif

	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	delete platform;
	
	return result;
}

void ReadScriptFromFile(std::string file_name)
{
	FILE * stream = nullptr;
	if (stream == nullptr)
	{
		printf("open file %s\n", file_name.c_str());
		stream = fopen(file_name.data(), "rb");
	}
	if (stream == nullptr)
	{
		printf("cannot find %s\n", file_name.c_str());
		return;
	}
	std::string str;
	if (str.empty())
	{
		fseek(stream, 0, SEEK_END);
		size_t len = ftell(stream);
		char *buffer = new char[len + 1];
		memset(buffer, 0, len + 1);
		fseek(stream, 0, SEEK_SET);
		fread(buffer, len, 1, stream);
		std::string str_temp(buffer, len);
		str = str_temp;
	}

	fclose(stream);

	file_script_map_[file_name] = str;
}

int main(int argc, char* argv[]) 
{
	ReadScriptFromFile("adsafe_dep.js");
	ReadScriptFromFile("big.js");
	ReadScriptFromFile("adsafe.js");
	ReadScriptFromFile("jslint.js");
	ReadScriptFromFile("test.js");
	return Repetition(argc, argv);
}


// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}


// Creates a new execution environment containing the built-in
// functions.
v8::Local<v8::Context> CreateShellContext(v8::Isolate* isolate) {
  // Create a template for the global object.
  v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
  // Bind the global 'print' function to the C++ Print callback.
  global->Set(
      v8::String::NewFromUtf8(isolate, "print", v8::NewStringType::kNormal)
          .ToLocalChecked(),
      v8::FunctionTemplate::New(isolate, Print));

  // Bind the global 'check_time' function to the C++ CheckTime callback.
  global->Set(
	  v8::String::NewFromUtf8(isolate, "internal_check_time", v8::NewStringType::kNormal)
	  .ToLocalChecked(),
	  v8::FunctionTemplate::New(isolate, CheckTime));

  // Bind the global 'check_time' function to the C++ Add callback.
  global->Set(
	  v8::String::NewFromUtf8(isolate, "internal_add", v8::NewStringType::kNormal)
	  .ToLocalChecked(),
	  v8::FunctionTemplate::New(isolate, Add));

  global->Set(
	  v8::String::NewFromUtf8(isolate, "include", v8::NewStringType::kNormal)
	  .ToLocalChecked(),
	  v8::FunctionTemplate::New(isolate, Include));

  // Bind the global 'read' function to the C++ Read callback.
  global->Set(v8::String::NewFromUtf8(
                  isolate, "read", v8::NewStringType::kNormal).ToLocalChecked(),
              v8::FunctionTemplate::New(isolate, Read));
  // Bind the global 'load' function to the C++ Load callback.
  global->Set(v8::String::NewFromUtf8(
                  isolate, "load", v8::NewStringType::kNormal).ToLocalChecked(),
              v8::FunctionTemplate::New(isolate, Load));
  // Bind the 'quit' function
  global->Set(v8::String::NewFromUtf8(
                  isolate, "quit", v8::NewStringType::kNormal).ToLocalChecked(),
              v8::FunctionTemplate::New(isolate, Quit));
  // Bind the 'version' function
  global->Set(
      v8::String::NewFromUtf8(isolate, "version", v8::NewStringType::kNormal)
          .ToLocalChecked(),
      v8::FunctionTemplate::New(isolate, Version));

  return v8::Context::New(isolate, NULL, global);
}


// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef RUN_UTEST
	return;
#endif
  bool first = true;
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    if (first) {
      first = false;
    } else {
      printf(" ");
    }

	{
		if (args[i]->IsObject()) {  //include map arrary
			v8::Local<v8::String> jsStr = v8::JSON::Stringify(args.GetIsolate()->GetCurrentContext(), args[i]->ToObject()).ToLocalChecked();
			std::string str = std::string(ToCString(v8::String::Utf8Value(jsStr)));
			LOG_INFO("%s\n", str.c_str());
		}
	}

    v8::String::Utf8Value str(args[i]);
    const char* cstr = ToCString(str);
    printf("%s", cstr);
  }
  printf("\n");
  fflush(stdout);
}

// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void CheckTime(const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifndef RUN_UTEST
	LOG_INFO("internal_check_time:%d, block:%d\n", g_check_all_time_count, g_check_block_time_count);
#endif

	v8::String::Utf8Value is_first_org(args[0]);
	std::string is_first_src = *is_first_org;
	bool is_first = (is_first_src.compare("true") == 0 )? true : false;
	if (is_first)
	{
		g_check_block_time_count++;
	}
	g_check_all_time_count++;
#ifdef USE_MEM_MONITOR
	v8::HeapStatistics stats;
	args.GetIsolate()->GetHeapStatistics(&stats);
	/*  printf("%u,%u,%u,%u,%u,%u,%u,%u,%u\n", stats.does_zap_garbage(), stats.heap_size_limit(), stats.malloced_memory(), stats.peak_malloced_memory(), stats.total_available_size(),
	stats.total_heap_size(), stats.total_heap_size_executable(), stats.total_physical_size(), stats.used_heap_size());*/
	//LOG_INFO("limite:%u, used:%u\n", (unsigned int)stats.heap_size_limit(), (unsigned int)stats.used_heap_size());
	//printf("context->EstimatedSize:%d\n", context->EstimatedSize());
#endif

	return;
}

void Add(const v8::FunctionCallbackInfo<v8::Value>& args) 
{
	if (args.Length() != 2) 
	{
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
			v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}

	char add_result[100] = {0};

	v8::String::Utf8Value add_1(args[0]);
	v8::String::Utf8Value add_2(args[1]);
	
	int64_t add_int64_result = Stoi64(*add_1) + Stoi64(*add_2);
#ifdef WIN32
	sprintf(add_result, "%lld", add_int64_result);
#else
	sprintf(add_result, "%ld", add_int64_result);
#endif

	v8::Local<v8::String> result(v8::String::NewFromUtf8(args.GetIsolate(), add_result, v8::NewStringType::kNormal).ToLocalChecked());
	args.GetReturnValue().Set(result);
	return;
}

void Include(const v8::FunctionCallbackInfo<v8::Value>& args) 
{	
	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	// Enter the execution environment before evaluating any code.
	v8::Context::Scope context_scope(context);

	if (args.Length() != 1) {
		LOG_INFO("Include parameter error, args length(%d) not equal 1\n", args.Length());
		args.GetReturnValue().Set(false);
		return;
	}

	if (!args[0]->IsString()) {
		LOG_INFO("Include parameter error, parameter should be a String\n");
		args.GetReturnValue().Set(false);
		return;
	}
	v8::String::Utf8Value str(args[0]);
	std::map<std::string, std::string>::iterator find_source = file_script_map_.find(*str);
	if (find_source == file_script_map_.end()) {
		LOG_INFO("Can't find the include file(%s) in jslib directory\n", *str);
		args.GetReturnValue().Set(false);
		return;
	}
	std::string js_file = find_source->second;

	v8::Local<v8::String> name(v8::String::NewFromUtf8(context->GetIsolate(), check_time, v8::NewStringType::kNormal).ToLocalChecked());
	ExecuteString(context->GetIsolate(), v8::String::NewFromUtf8(context->GetIsolate(), js_file.data(), v8::NewStringType::kNormal).ToLocalChecked(), name, true, true);
}


// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
                                v8::NewStringType::kNormal).ToLocalChecked());
    return;
  }
  v8::String::Utf8Value file(args[0]);
  if (*file == NULL) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                v8::NewStringType::kNormal).ToLocalChecked());
    return;
  }
  v8::Local<v8::String> source;
  if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                v8::NewStringType::kNormal).ToLocalChecked());
    return;
  }
  args.GetReturnValue().Set(source);
}


// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
void Load(const v8::FunctionCallbackInfo<v8::Value>& args) {
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    v8::String::Utf8Value file(args[i]);
    if (*file == NULL) {
      args.GetIsolate()->ThrowException(
          v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                  v8::NewStringType::kNormal).ToLocalChecked());
      return;
    }
    v8::Local<v8::String> source;
    if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
      args.GetIsolate()->ThrowException(
          v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                  v8::NewStringType::kNormal).ToLocalChecked());
      return;
    }
    if (!ExecuteString(args.GetIsolate(), source, args[i], false, false)) {
      args.GetIsolate()->ThrowException(
          v8::String::NewFromUtf8(args.GetIsolate(), "Error executing file",
                                  v8::NewStringType::kNormal).ToLocalChecked());
      return;
    }
  }
}


// The callback that is invoked by v8 whenever the JavaScript 'quit'
// function is called.  Quits.
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args) {
  // If not arguments are given args[0] will yield undefined which
  // converts to the integer value 0.
  int exit_code =
      args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
  fflush(stdout);
  fflush(stderr);
  exit(exit_code);
}


void Version(const v8::FunctionCallbackInfo<v8::Value>& args) {
  args.GetReturnValue().Set(
      v8::String::NewFromUtf8(args.GetIsolate(), v8::V8::GetVersion(),
                              v8::NewStringType::kNormal).ToLocalChecked());
}


// Reads a file into a v8 string.
v8::MaybeLocal<v8::String> ReadFile(v8::Isolate* isolate, const char* name) {
  FILE* file = fopen(name, "rb");
  if (file == NULL) return v8::MaybeLocal<v8::String>();

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (size_t i = 0; i < size;) {
    i += fread(&chars[i], 1, size - i, file);
    if (ferror(file)) {
      fclose(file);
      return v8::MaybeLocal<v8::String>();
    }
  }
  fclose(file);
  v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(
      isolate, chars, v8::NewStringType::kNormal, static_cast<int>(size));
  delete[] chars;
  return result;
}


// Process remaining command line arguments and execute files
int RunMain(v8::Isolate* isolate, v8::Platform* platform, int argc,
            char* argv[]) {
  for (int i = 1; i < argc; i++) {
    const char* str = argv[i];
    if (strcmp(str, "--shell") == 0) {
      run_shell = true;
    } else if (strcmp(str, "-f") == 0) {
      // Ignore any -f flags for compatibility with the other stand-
      // alone JavaScript engines.
      continue;
    } else if (strncmp(str, "--", 2) == 0) {
      fprintf(stderr,
              "Warning: unknown flag %s.\nTry --help for options\n", str);
    } else if (strcmp(str, "-e") == 0 && i + 1 < argc) {
      // Execute argument given to -e option directly.
      v8::Local<v8::String> file_name =
          v8::String::NewFromUtf8(isolate, "unnamed",
                                  v8::NewStringType::kNormal).ToLocalChecked();
      v8::Local<v8::String> source;
      if (!v8::String::NewFromUtf8(isolate, argv[++i],
                                   v8::NewStringType::kNormal)
               .ToLocal(&source)) {
        return 1;
      }
      bool success = ExecuteString(isolate, source, file_name, false, true);
      while (v8::platform::PumpMessageLoop(platform, isolate)) continue;
      if (!success) return 1;
    } else {
      // Use all other arguments as names of files to load and run.
      v8::Local<v8::String> file_name =
          v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kNormal)
              .ToLocalChecked();
      v8::Local<v8::String> source;
      if (!ReadFile(isolate, str).ToLocal(&source)) {
        fprintf(stderr, "Error reading '%s'\n", str);
        continue;
      }
      bool success = ExecuteString(isolate, source, file_name, false, true);
      while (v8::platform::PumpMessageLoop(platform, isolate)) continue;
      if (!success) return 1;
    }
  }
  return 0;
}


// The read-eval-execute loop of the shell.
void RunShell(v8::Local<v8::Context> context, v8::Platform* platform) {
  fprintf(stderr, "V8 version %s [sample shell]\n", v8::V8::GetVersion());
  static const int kBufferSize = 100 * 1000;
  // Enter the execution environment before evaluating any code.
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> name(
	  v8::String::NewFromUtf8(context->GetIsolate(), check_time,
                              v8::NewStringType::kNormal).ToLocalChecked());
  while (true) {
    char buffer[kBufferSize];
    fprintf(stderr, "> ");
    char* str = fgets(buffer, kBufferSize, stdin);
    if (str == NULL) break;
	std::string str_target(str);
	if (str_target.compare("exit\n") == 0) break;
    v8::HandleScope handle_scope(context->GetIsolate());
    ExecuteString(
        context->GetIsolate(),
        v8::String::NewFromUtf8(context->GetIsolate(), str,
                                v8::NewStringType::kNormal).ToLocalChecked(),
        name, true, true);
    while (v8::platform::PumpMessageLoop(platform, context->GetIsolate()))
      continue;
  }
  fprintf(stderr, "\n");
}

void RunUtest(v8::Local<v8::Context> context, v8::Platform* platform)
{
	typedef struct tagSplitCase
	{
		int expect_block_result;
		int expect_all_result;
		std::string str;
	}SplitCase;
	static std::vector<SplitCase> split_cases;
	
	if (split_cases.empty())
	{
		std::string file_name = "v8-utest.txt";
		static FILE * stream = fopen(file_name.data(), "r");
		
		if (stream == nullptr)
		{
			printf("cannot find v8-utest.txt\n");
			return;
		}
		fseek(stream, 0, SEEK_SET);

		std::vector<std::string> script_vector;
		char line[10000];
		while (fgets(line, 10000, stream) != NULL)
		{
			//printf("line:%s\n", line);
			script_vector.push_back(line);
		}

		for (unsigned int i = 0; i < script_vector.size(); i++)
		{
			std::string scrpit_all = script_vector[i];
			size_t idx = scrpit_all.find(',');
			if (idx == std::string::npos)
			{
				printf("error script [index:%d] :%s", i + 1, scrpit_all.c_str());
				continue;
			}
			SplitCase one_case;
			one_case.expect_block_result = atoi(scrpit_all.substr(0, idx).data());
			size_t idx_2 = scrpit_all.find(',', idx + 1);
			if (idx_2 == std::string::npos)
			{
				printf("error script [index:%d] :%s", i + 1, scrpit_all.c_str());
				continue;
			}
			one_case.expect_all_result = atoi(scrpit_all.substr(idx + 1, idx_2 + 1).data());
			one_case.str = scrpit_all.substr(idx_2 + 1, std::string::npos);
			split_cases.push_back(one_case);
		}
	}

	LOG_INFO("-----------------------Utest Start------------------------------------------------\n");
	static const int kBufferSize = 10 * 1024;
	// Enter the execution environment before evaluating any code.
	for (unsigned int i = 0; i < split_cases.size(); i++)
	{
		const SplitCase &one_case = split_cases[i];
		int expect_block_result = one_case.expect_block_result;
		int expect_all_result = one_case.expect_all_result;
		std::string script_src = one_case.str;

		v8::Context::Scope context_scope(context);
		v8::Local<v8::String> name(
			v8::String::NewFromUtf8(context->GetIsolate(), check_time,
			v8::NewStringType::kNormal).ToLocalChecked());

		char buffer[kBufferSize];
		//fprintf(stderr, "> ");
		v8::HandleScope handle_scope(context->GetIsolate());
		ExecuteString(
			context->GetIsolate(),
			v8::String::NewFromUtf8(context->GetIsolate(), script_src.data(),
			v8::NewStringType::kNormal).ToLocalChecked(),
			name, true, true);
		
		if ((g_check_block_time_count == (unsigned int)expect_block_result) 
			&& (g_check_all_time_count == (unsigned int)expect_all_result))
		{
			LOG_INFO("-----------------------Utest [%d] OK.[expect,true] [%d,%u] [%d, %u]----------------------\n", 
				i + 1, expect_block_result, g_check_block_time_count, expect_all_result, g_check_all_time_count);
		}
		else
		{
#ifdef WIN32
			HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | FOREGROUND_RED);
#endif
			LOG_INFO("-----------------------Src: %s\n", script_src.c_str());
			LOG_INFO("-----------------------Utest [%d] ERR.[expect,true] [%d,%u] [%d, %u]----------------------\n",
				i + 1, expect_block_result, g_check_block_time_count, expect_all_result, g_check_all_time_count);
			g_check_block_time_count = 0;
			g_check_all_time_count = 0;
			break;
		}
		//assert(g_check_block_time_count == expect_result);
		
		g_check_block_time_count = 0;
		g_check_all_time_count = 0;
	}

	LOG_INFO("\n-----------------------Utest End------------------------------------------------\n");
}

// Executes a string within the current v8 context.
bool ExecuteString(v8::Isolate* isolate, v8::Local<v8::String> source,
                   v8::Local<v8::Value> name, bool print_result,
                   bool report_exceptions) {
  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch(isolate);
  v8::ScriptOrigin origin(name);
  v8::Local<v8::Context> context(isolate->GetCurrentContext());
  
  v8::Local<v8::Script> script;
  if (!v8::Script::Compile(context, source, &origin).ToLocal(&script)) {
    // Print errors that happened during compilation.
    if (report_exceptions)
      ReportException(isolate, &try_catch);
    return false;
  } else {
    v8::Local<v8::Value> result;
    if (!script->Run(context).ToLocal(&result)) {
      assert(try_catch.HasCaught());
      // Print errors that happened during execution.
      if (report_exceptions)
        ReportException(isolate, &try_catch);
      return false;
    } else {
      assert(!try_catch.HasCaught());
      if (print_result && !result->IsUndefined()) {
        // If all went well and the result wasn't undefined then print
        // the returned value.
        v8::String::Utf8Value str(result);
        const char* cstr = ToCString(str);
        //printf("run result:  %s\n", cstr);
      }
	  v8::HeapStatistics stats;
	  isolate->GetHeapStatistics(&stats);
	  /*  printf("%u,%u,%u,%u,%u,%u,%u,%u,%u\n", stats.does_zap_garbage(), stats.heap_size_limit(), stats.malloced_memory(), stats.peak_malloced_memory(), stats.total_available_size(),
			stats.total_heap_size(), stats.total_heap_size_executable(), stats.total_physical_size(), stats.used_heap_size());*/
	  //printf("limite:%u, used:%u\n", stats.heap_size_limit(), stats.used_heap_size());
	  //printf("context->EstimatedSize:%d\n", context->EstimatedSize());
      return true;
    }
	//isolate->RequestInterrupt();
	
  }
}


void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
  v8::HandleScope handle_scope(isolate);
  v8::String::Utf8Value exception(try_catch->Exception());
  const char* exception_string = ToCString(exception);
  v8::Local<v8::Message> message = try_catch->Message();
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    fprintf(stderr, "%s\n", exception_string);
  } else {
    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
    v8::Local<v8::Context> context(isolate->GetCurrentContext());
    const char* filename_string = ToCString(filename);
    int linenum = message->GetLineNumber(context).FromJust();
    fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
    // Print line of source code.
    v8::String::Utf8Value sourceline(
        message->GetSourceLine(context).ToLocalChecked());
    const char* sourceline_string = ToCString(sourceline);
    fprintf(stderr, "%s\n", sourceline_string);
    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn(context).FromJust();
    for (int i = 0; i < start; i++) {
      fprintf(stderr, " ");
    }
    int end = message->GetEndColumn(context).FromJust();
    for (int i = start; i < end; i++) {
      fprintf(stderr, "^");
    }
    fprintf(stderr, "\n");
    v8::Local<v8::Value> stack_trace_string;
    if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
        stack_trace_string->IsString() &&
        v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
      v8::String::Utf8Value stack_trace(stack_trace_string);
      const char* stack_trace_string = ToCString(stack_trace);
      fprintf(stderr, "%s\n", stack_trace_string);
    }
  }
}
