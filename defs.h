#pragma once

#define yesno(b) ((b) ? "yes" : "no")

#define traceprintf                                                            \
  printf("%s:%d:%s ", __FILE__, __LINE__, __FUNCTION__), printf

// -----------------------------------------------------------------

class Trace {
public:
  Trace(bool const trace, char const *const function, char const *const file,
        int const line);
  ~Trace();

private:
  void _trace(char const lead[]);

  bool const _output;
  char const *const _function;
  char const *const _file;
  int const _line;
}; // class Trace

#if defined(_DEBUG) || defined(_DEBUG)
#define TRACE(trace) Trace __trace(trace, __FUNCTION__, __FILE__, __LINE__)
#else // _DEBUG _DEBUG
#define TRACE(trace) (void)(0)
#endif // _DEBUG _DEBUG
