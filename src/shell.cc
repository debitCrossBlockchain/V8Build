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
int g_repetition_count = 1;
int g_check_time_count = 0;

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
void Include(const v8::FunctionCallbackInfo<v8::Value>& args);
void Read(const v8::FunctionCallbackInfo<v8::Value>& args);
void Load(const v8::FunctionCallbackInfo<v8::Value>& args);
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);
void Version(const v8::FunctionCallbackInfo<v8::Value>& args);
v8::MaybeLocal<v8::String> ReadFile(v8::Isolate* isolate, const char* name);
void ReportException(v8::Isolate* isolate, v8::TryCatch* handler);


static bool run_shell;

int Repetition(int argc, char* argv[])
{
	v8::V8::InitializeICUDefaultLocation(argv[0]);
	v8::V8::InitializeExternalStartupData(argv[0]);
	v8::Platform* platform = v8::platform::CreateDefaultPlatform();
	v8::V8::InitializePlatform(platform);
	v8::V8::Initialize();
	v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator =
		v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	v8::Isolate* isolate = v8::Isolate::New(create_params);

	int result;
	for (int i = 0; i < g_repetition_count; i++)
	{
#ifdef WIN32
		Sleep(10);
#else
		usleep(10 * 1000);
#endif
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

	char buffer[10000];
	char* str = fgets(buffer, 10000, stdin);

	isolate->Dispose();
	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	delete platform;
	delete create_params.array_buffer_allocator;
	return result;
}

int main(int argc, char* argv[]) 
{
	return Repetition(argc, argv);;
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
	printf("internal_check_time\n");
#endif
	g_check_time_count++;

	return;
}

void Include(const v8::FunctionCallbackInfo<v8::Value>& args) 
{	
	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	// Enter the execution environment before evaluating any code.
	v8::Context::Scope context_scope(context);

	std::string file_name = "D:\\Workspace\\v8\\v8-2018-01-04\\v8\\samples\\big.js";
	static FILE * stream = fopen(file_name.data(), "rb");
	if (stream == nullptr)
	{
		printf("cannot find %s\n", file_name.c_str());
		return;
	}
	fseek(stream, 0, SEEK_END);
	size_t len = ftell(stream);
	char *buffer = new char[len + 1];
	memset(buffer, 0, len + 1);
	fseek(stream, 0, SEEK_SET);
	fread(buffer, len, 1, stream);
	//__enable_check_time__
	v8::Local<v8::String> name(v8::String::NewFromUtf8(context->GetIsolate(), "__enable_check_time__", v8::NewStringType::kNormal).ToLocalChecked());
	std::string str(buffer, len);
	ExecuteString(context->GetIsolate(), v8::String::NewFromUtf8(context->GetIsolate(), str.data(), v8::NewStringType::kNormal).ToLocalChecked(), name, true, true);
	
	//do {

	//	v8::TryCatch try_catch(args.GetIsolate());
	//	std::string js_file = "";

	//	v8::Local<v8::String> source = v8::String::NewFromUtf8(args.GetIsolate(), js_file.c_str());
	//	v8::Local<v8::Script> script;

	//	v8::Local<v8::String> check_time_name(
	//		v8::String::NewFromUtf8(args.GetIsolate()->GetCurrentContext()->GetIsolate(), "__enable_check_time__",
	//		v8::NewStringType::kNormal).ToLocalChecked());
	//	v8::ScriptOrigin origin_check_time_name(check_time_name);

	//	if (!v8::Script::Compile(args.GetIsolate()->GetCurrentContext(), source, &origin_check_time_name).ToLocal(&script)) {
	//		ReportException(args.GetIsolate(), &try_catch);
	//		break;
	//	}

	//	v8::Local<v8::Value> result;
	//	if (!script->Run(args.GetIsolate()->GetCurrentContext()).ToLocal(&result)) {
	//		ReportException(args.GetIsolate(), &try_catch);
	//	}
	//} while (false);
	//return v8::Undefined(args.GetIsolate());
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
      v8::String::NewFromUtf8(context->GetIsolate(), "__enable_check_time__",
                              v8::NewStringType::kNormal).ToLocalChecked());
  while (true) {
    char buffer[kBufferSize];
    fprintf(stderr, "> ");
    char* str = fgets(buffer, kBufferSize, stdin);
    if (str == NULL) break;
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
		int expect_result;
		std::string str;
	}SplitCase;
	static std::vector<SplitCase> split_cases;
	
	if (split_cases.empty())
	{
		//std::string file_name = "v8-utest.txt";
		std::string file_name = "D:\\Workspace\\v8\\v8-2018-01-04\\v8\\build\\Debug\\v8-utest.txt";
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
			one_case.expect_result = atoi(scrpit_all.substr(0, idx).data());
			one_case.str = scrpit_all.substr(idx + 1, std::string::npos);
			split_cases.push_back(one_case);
		}
	}

	fprintf(stderr, "-----------------------Utest Start------------------------------------------------\n");
	static const int kBufferSize = 10 * 1024;
	// Enter the execution environment before evaluating any code.
	for (unsigned int i = 0; i < split_cases.size(); i++)
	{
		const SplitCase &one_case = split_cases[i];
		int expect_result = one_case.expect_result;
		std::string script_src = one_case.str;

		v8::Context::Scope context_scope(context);
		v8::Local<v8::String> name(
			v8::String::NewFromUtf8(context->GetIsolate(), "__enable_check_time__",
			v8::NewStringType::kNormal).ToLocalChecked());

		char buffer[kBufferSize];
		//fprintf(stderr, "> ");
		v8::HandleScope handle_scope(context->GetIsolate());
		ExecuteString(
			context->GetIsolate(),
			v8::String::NewFromUtf8(context->GetIsolate(), script_src.data(),
			v8::NewStringType::kNormal).ToLocalChecked(),
			name, true, true);
		
		if (g_check_time_count == expect_result)
		{
			printf("-----------------------Utest [%d] OK.[expect:%d, true result:%d]----------------------\n", i + 1, expect_result, g_check_time_count);
		}
		else
		{
#ifdef WIN32
			HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
			SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY | FOREGROUND_RED);
#endif
			printf("-----------------------Src: %s\n", script_src.c_str());
			printf("-----------------------Utest [%d] ERR.[expect:%d, true result:%d]----------------------\n", i + 1, expect_result, g_check_time_count);
			break;
		}
		//assert(g_check_time_count == expect_result);
		
		g_check_time_count = 0;
	}

	fprintf(stderr, "\n-----------------------Utest End------------------------------------------------\n");
}

// Executes a string within the current v8 context.
bool ExecuteString(v8::Isolate* isolate, v8::Local<v8::String> source,
                   v8::Local<v8::Value> name, bool print_result,
                   bool report_exceptions) {
  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch(isolate);
  v8::ScriptOrigin origin(name);
  v8::Local<v8::Context> context(isolate->GetCurrentContext());
  context->SetEmbedderData(8888, v8::Number::New(isolate, 8888));
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
      return true;
    }
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
