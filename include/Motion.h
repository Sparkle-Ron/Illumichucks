#pragma once
namespace Motion {
struct Sample { float x, y, z; };
enum class ReadResult { Fresh, NotReady, Error };
bool begin();
bool configureActive();
ReadResult read(Sample& sample);
bool prepareWake();
bool wakePending();
}
