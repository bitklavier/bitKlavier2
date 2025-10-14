/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cmath>
#include <complex>
#include <cstdlib>
#include <random>

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "common.h"
#include "BKADSR.h"

static constexpr int MaxMidiNotes = 128;

#define NAME_AND_VALUE(x) #x, x

  namespace bitklavier::utils {

    constexpr float kDbGainConversionMult = 20.0f;
    constexpr int kMaxOrderLength = 10;
    constexpr float kLogOf2 = 0.69314718056f;
    constexpr float kInvLogOf2 = 1.44269504089f;

    constexpr int factorial(int value) {
      int result = 1;
      for (int i = 2; i <= value; ++i)
        result *= i;

      return result;
    }

    typedef union {
      int i;
      float f;
    } int_float;

    class RandomGenerator {
      public:
        static int next_seed_;
          
        RandomGenerator(float min, float max) : engine_(next_seed_++), distribution_(min, max) { }
        RandomGenerator(const RandomGenerator& other) :
            engine_(next_seed_++), distribution_(other.distribution_.min(), other.distribution_.max()) { }

        force_inline float next() {
          return distribution_(engine_);
        }

        force_inline void seed(int new_seed) {
          engine_.seed(new_seed);
        }

      private:
        std::mt19937 engine_;
        std::uniform_real_distribution<float> distribution_;

        JUCE_LEAK_DETECTOR(RandomGenerator)
    };

    force_inline float intToFloatBits(int i) {
      int_float convert;
      convert.i = i;
      return convert.f;
    }

    force_inline int floatToIntBits(float f) {
      int_float convert;
      convert.f = f;
      return convert.i;
    }

    force_inline float min(float one, float two) {
      return fmin(one, two);
    }

    force_inline float max(float one, float two) {
      return fmax(one, two);
    }

    force_inline float clamp(float value, float min, float max) {
      return fmin(max, fmax(value, min));
    }

    template<class T>
    force_inline T pass(T input) {
      return input;
    }

    force_inline int imax(int one, int two) {
      return (one > two) ? one : two;
    }

    force_inline int imin(int one, int two) {
      return (one > two) ? two : one;
    }

    force_inline double interpolate(double from, double to, double t) {
      return t * (to - from) + from;
    }

    force_inline float interpolate(float from, float to, float t) {
      return from + t * (to - from);
    }
    
    force_inline float mod(double value, double* divisor) {
      return modf(value, divisor);
    }

    force_inline float mod(float value, float* divisor) {
      return modff(value, divisor);
    }

    force_inline int mod(int a, int b) { return (a % b + b) % b; }

    force_inline int iclamp(int value, int min, int max) {
      return value > max ? max : (value < min ? min : value);
    }

    force_inline int ilog2(int value) {
    #if defined(__GNUC__) || defined(__clang__)
      constexpr int kMaxBitIndex = sizeof(int) * 8 - 1;
      return kMaxBitIndex - __builtin_clz(std::max(value, 1));
    #elif defined(_MSC_VER)
      unsigned long result = 0;
      _BitScanReverse(&result, value);
      return result;
    #else
      int num = 0;
      while (value >>= 1)
        num++;
      return num;
    #endif
    }

    force_inline bool closeToZero(float value) {
      return value <= kEpsilon && value >= -kEpsilon;
    }

    force_inline float magnitudeToDb(float magnitude) {
      return kDbGainConversionMult * log10f(magnitude);
    }

    force_inline float dbToMagnitude(float decibels) {
      return powf(10.0f, decibels / kDbGainConversionMult);
    }

    force_inline float centsToRatio(float cents) {
      return powf(2.0f, cents / kCentsPerOctave);
    }

    force_inline float noteOffsetToRatio(float cents) {
      return powf(2.0f, cents / kNotesPerOctave);
    }

    force_inline float ratioToMidiTranspose(float ratio) {
      return logf(ratio) * (kInvLogOf2 * kNotesPerOctave);
    }

    force_inline float midiCentsToFrequency(float cents) {
      return kMidi0Frequency * centsToRatio(cents);
    }

    force_inline float midiNoteToFrequency(float note) {
      return midiCentsToFrequency(note * kCentsPerNote);
    }

    force_inline float frequencyToMidiNote(float frequency) {
      return kNotesPerOctave * logf(frequency / kMidi0Frequency) * kInvLogOf2;
    }

    force_inline float frequencyToMidiCents(float frequency) {
      return kCentsPerNote * frequencyToMidiNote(frequency);
    }

    force_inline int nextPowerOfTwo(float value) {
      return roundf(powf(2.0f, ceilf(logf(value) * kInvLogOf2)));
    }

    force_inline bool isSilent(const float* buffer, int length) {
      for (int i = 0; i < length; ++i) {
        if (!closeToZero(buffer[i]))
          return false;
      }
      return true;
    }

    force_inline float rms(const float* buffer, int num) {
      float square_total = 0.0f;
      for (int i = 0; i < num; ++i)
        square_total += buffer[i] * buffer[i];

      return sqrtf(square_total / num);
    }

    force_inline float inversePowerScale(float t) {
      return 2.0f * logf((-t + 1.0f) / t);
    }

    force_inline float inverseFltScale(float t) {
      return (t - 1.0f) / t;
    }

    float encodeOrderToFloat(int* order, int size);
    void decodeFloatToOrder(int* order, float float_code, int size);
    void floatToPcmData(int16_t* pcm_data, const float* float_data, int size);
    void complexToPcmData(int16_t* pcm_data, const std::complex<float>* complex_data, int size);
    void pcmToFloatData(float* float_data, const int16_t* pcm_data, int size);
    void pcmToComplexData(std::complex<float>* complex_data, const int16_t* pcm_data, int size);

    typedef enum BKSampleLoadType
    {
        BKLoadDefault = 0,
        BKLoadSampleSet2,
        BKLoadSampleSet3,
        BKLoadSampleSet4,
        BKNumSampleLoadTypes
    } BKSampleLoadType;

    const std::string samplepaths[BKSampleLoadType::BKNumSampleLoadTypes]
        {
         "/default",
         "/sampleset2", // made up for now...
         "/sampleset3",
         "/sampleset4"
        };

    typedef enum BKPianoSampleType
    {
        BKPianoMain = 0,
        BKPianoHammer,
        BKPianoReleaseResonance,
        BKPianoPedal,
        BKPianoSampleTypes
    } BKPianoSampleType;

    const std::string BKPianoSampleType_string[BKPianoSampleType::BKPianoSampleTypes]
    {
        "/main",
        "/hammer",
        "/resonance",
        "/pedal"
    };

// Template structure to define velocity ranges based on the number
      // Primary template declaration
      template<size_t N>
      struct VelocityRange
      {
          static constexpr std::array<std::tuple<int, int>, N> values; // Declaration
      };

// Initialize the primary template's static member (empty in this case)
//      template<int N>
//      constexpr std::array<std::tuple<int, int>, N> VelocityRange<N>::values = {};


      template<>
      constexpr std::array<std::tuple<int, int>, 16> VelocityRange<16>::values = {
          std::make_tuple(0, 8),
          std::make_tuple(8, 16),
          std::make_tuple(16, 24),
          std::make_tuple(24, 32),
          std::make_tuple(32, 40),
          std::make_tuple(40, 48),
          std::make_tuple(48, 56),
          std::make_tuple(56, 64),
          std::make_tuple(64, 72),
          std::make_tuple(72, 80),
          std::make_tuple(80, 88),
          std::make_tuple(88, 96),
          std::make_tuple(96, 104),
          std::make_tuple(104, 112),
          std::make_tuple(112, 120),
          std::make_tuple(120, 128)
      };
      // Define the tuples for N = 8
      template<>
      constexpr std::array<std::tuple<int, int>, 8> VelocityRange<8>::values = {
          std::make_tuple(0, 30),
          std::make_tuple(30, 50),
          std::make_tuple(50, 68),
          std::make_tuple(68, 84),
          std::make_tuple(84, 98),
          std::make_tuple(98, 110),
          std::make_tuple(110, 120),
          std::make_tuple(120, 128)
      };

      // Define the tuples for N = 4
      template<>
      constexpr std::array<std::tuple<int, int>, 4> VelocityRange<4>::values = {
          std::make_tuple(0, 42),
          std::make_tuple(42, 76),
          std::make_tuple(76, 104),
          std::make_tuple(104, 128)
      };


      // Define the tuples for N = 2
      template<>
      constexpr std::array<std::tuple<int, int>, 2> VelocityRange<2>::values = {
          std::make_tuple(0, 76),
          std::make_tuple(76, 128)
      };


      // Define the tuples for N = 1
      template<>
      constexpr std::array<std::tuple<int, int>, 1> VelocityRange<1>::values = {
          std::make_tuple(0, 128)
      };
  

      double dt_asymwarp(double inval, double k);
      double dt_asymwarp_inverse(double inval, double k);
      double dt_symwarp(double inval, double k);
      double dt_warpscale(double inval, double asym_k, double sym_k, double scale, double offset);

      std::bitset<128> stringToBitset (juce::String paramAttribute);
  } // namespace bitklavier::utils

// namespace vital

// standard filename comparator, though doesn't handle velocities well
class MyComparator
{
public:
    static int compareElements (juce::File first, juce::File second) {
        if (first.getFileNameWithoutExtension() < second.getFileNameWithoutExtension())
            return -1;
        else if (first.getFileNameWithoutExtension() < second.getFileNameWithoutExtension())
            return 1;
        else
            return 0; //items are equal
    }
};

// finds velocities in sample names and uses those to sort
class VelocityComparator
{
public:
    static int compareElements (juce::File first, juce::File second) {
        // assumes sample names of form [PitchLetter(s)][octave]v[velocity].wav
        //      so A3v13.wav, of C#0v2.wav
        //      with only sharps, and ABCDEFG
        // find where the 'v' is, and where the '.' is for the suffix
        auto vIndexFirst    = first.getFileName().indexOfChar('v') + 1;
        auto vIndexSecond   = second.getFileName().indexOfChar('v') + 1;
        auto vEndIndexFirst = first.getFileName().indexOfChar('.');
        auto vEndIndexSecond= second.getFileName().indexOfChar('.');

        // get the velocity as an int
        int velocityFirst = first.getFileName().substring(vIndexFirst, vEndIndexFirst).getIntValue();
        int velocitySecond = second.getFileName().substring(vIndexSecond, vEndIndexSecond).getIntValue();

        // compare as ints
        if (velocityFirst < velocitySecond)
            return -1;
        else if (velocityFirst < velocitySecond)
            return 1;
        else
            return 0; //items are equal
    }
};

static inline int noteNameToRoot(juce::String name)
{
    int root = 0;
    if (name[0] == 'C') root = 0;
    else if (name[0] == 'D') root = 2;
    else if (name[0] == 'E') root = 4;
    else if (name[0] == 'F') root = 5;
    else if (name[0] == 'G') root = 7;
    else if (name[0] == 'A') root = 9;
    else if (name[0] == 'B') root = 11;

    if (name[1] == '#') root++;
    else if (name[1] == 'b') root--;

    root += 12 * name.getTrailingIntValue() + 12;

    return root;
}

/**
 * Midi to Frequency convertor
 * @param f
 * @return freq (Hertz)
 */
static double mtof( double f )
{
    if( f <= -1500 ) return (0);
    else if( f > 1499 ) return (mtof(1499));
    // else return (8.17579891564 * exp(.0577622650 * f));
    // TODO: optimize
    else return ( pow(2, (f - 69) / 12.0) * 440.0 );
}

/**
 * Midi to Frequency convertor
 * @param f
 * @param a = concert pitch
 * @return freq (Hertz)
 */
static double mtof( double f, double a ) // a = frequency of A4
{
    if( f <= -1500 ) return (0);
    else if( f > 1499 ) return (mtof(1499));
    // else return (8.17579891564 * exp(.0577622650 * f));
    // TODO: optimize
    else return ( pow(2, (f - 69) / 12.0) * a );
}
/* better than...
 *
 * float mtof(const float midiNote) // converts midiPitch to frequency in Hz
{
    return concertPitchHz * std::pow(2.0f, ((midiNote - 69.0f) / 12.0f));
};
 *  ?
 */

/**
 * Frequency to Midi convertory
 * @param inputFreq
 * @param concertPitchHz
 * @return
 */
//static double ftom(const double inputFreq, double concertPitchHz)
//{
//    return 12.0f * log2(inputFreq / concertPitchHz) + 69.0f;
//};

//-----------------------------------------------------------------------------
// name: ftom()
// desc: freq to midi
//-----------------------------------------------------------------------------
#define LOGTWO 0.69314718055994528623
//#define LOGTEN 2.302585092994
//static double ftom(const double f )
//{
//    // return (f > 0 ? 17.3123405046 * log(.12231220585 * f) : -1500);
//    // TODO: optimize
//    return (f > 0 ? (log(f / 440.0) / LOGTWO) * 12.0 + 69. : -1500);
//}

static double ftom(const double f, double a440 )
{
    // TODO: optimize
    return (f > 0 ? (log(f / a440) / LOGTWO) * 12.0 + 69. : -1500);
}

/**
 * struct to put information about the last state of the synth
 * at the end of each block. useful for UI...
 * todo: remove lastVelocity if not needed after moving velocity min/max to Keymap
 */
struct BKSynthesizerState
{
    double lastPitch;
};

/*
 * additional specifications to associate with a particular noteOn msg
 *  - usually included in a std::map, keyed by midiNoteNumber
 */
struct NoteOnSpec
{
    bool keyState = false;                          // turn on for notes that should use the extra specs here
    float startTime = 0.f;                          // where to start playback (ms)
    Direction startDirection = Direction::forward;  // direction
    LoopMode loopMode = LoopMode::none;             // currently we don't use loopmode, but perhaps some day
    bool stopSameCurrentNote = true;                // if this note is playing already, stop it (default behavior)
    BKADSR::Parameters envParams {3.0f * .001, 10.0f * .001, 1.0f, 50.0f * .001, 0.0f, 0.0f, 0.0f}; // BKADSR time values are in seconds

    void clear()
    {
        keyState = false;                          // turn on for notes that should use the extra specs here
        startTime = 0.f;                          // where to start playback (ms)
        startDirection = Direction::forward;  // direction
        loopMode = LoopMode::none;             // currently we don't use loopmode, but perhaps some day
        stopSameCurrentNote = true;                // if this note is playing already, stop it (default behavior)
        envParams = {3.0f * .001, 10.0f * .001, 1.0f, 50.0f * .001, 0.0f, 0.0f, 0.0f}; // BKADSR time values are in seconds
    }
};

/*
 * sharp/flat mappings use substitutions in std::initializer_list
 * as part of the param definition (reffundamental, for instance)
 */
enum class PitchClass : uint32_t {
    C = 1 << 0,
    C41D5 = 1 << 1, // substitutions: 4 => #, 1=>/, 5 => b, so C41D5 becomes "C#/Db"
    D = 1 << 2,
    D41E5 = 1 << 3,
    E = 1 << 4,
    F = 1 << 5,
    F41G5 = 1 << 6,
    G = 1 << 7,
    G41A5 = 1 << 8,
    A = 1 << 9,
    A41B5 = 1 << 10,
    B = 1 << 11,
    PitchClassNil
};

enum class Fundamental : uint32_t {
    C = 1 << 0,
    C41D5 = 1 << 1, // substitutions: 4 => #, 1=>/, 5 => b, so C41D5 becomes "C#/Db"
    D = 1 << 2,
    D41E5 = 1 << 3,
    E = 1 << 4,
    F = 1 << 5,
    F41G5 = 1 << 6,
    G = 1 << 7,
    G41A5 = 1 << 8,
    A = 1 << 9,
    A41B5 = 1 << 10,
    B = 1 << 11,
    none = 1 << 12,
    lowest = 1 << 13,
    highest = 1 << 14,
    last = 1 << 15,
    automatic = 1 << 16,
    FundamentalNil
};

enum Octave : uint32_t {
    _0 = 1 << 0, // tricked this enum into displaying integers (_ => space)
    _1 = 1 << 1,
    _2 = 1 << 2,
    _3 = 1 << 3,
    _4 = 1 << 4,
    _5 = 1 << 5,
    _6 = 1 << 6,
    _7 = 1 << 7,
    _8 = 1 << 8,
};

enum TuningType {
    Static = 1 << 0,
    Adaptive = 1 << 1,
    Adaptive_Anchored = 1 << 2,
    Spring_Tuning = 1 << 3,
};

enum SelectChoice {
    Select = 1 << 0,
    Deselect = 1 << 1,
};

struct KeymapKeyboardState
{
    KeymapKeyboardState() {
        keyStates.reset();
    }

    std::bitset<128> keyStates;
};

template <size_t N>
size_t find_first_set_bit(const std::bitset<N>& bs)
{
    for (size_t i = 0; i < bs.size(); ++i)
    {
        if (bs.test(i))
        {
            return i; // Found the index of the first '1'
        }
    }
    return N; // No set bits found
}

template <typename T, std::size_t N>
void insert_and_shift(std::array<T, N>& arr, const T& new_value)
{
    // Check for an empty array, although N is usually known at compile time.
    if constexpr (N > 0)
    {
        // 1. Shift elements back one index.
        //    std::copy(source_begin, source_end, destination_begin);
        //    We copy from arr[0] up to arr[N-2]
        //    TO arr[1] up to arr[N-1]
        //    This effectively discards the value at arr[N-1] (the end).

        // This is safe because the source range (arr.begin() to arr.end() - 1)
        // and the destination range (arr.begin() + 1 to arr.end())
        // do not overlap in a way that causes corruption (it's a backward shift).
        std::copy(
            arr.begin(),          // Source start (arr[0])
            arr.end() - 1,        // Source end (one past arr[N-2])
            arr.begin() + 1       // Destination start (arr[1])
        );

        // 2. Insert the new value at the beginning.
        arr[0] = new_value;
    }
}

template <typename T, std::size_t N>
std::size_t remove_all_and_compact(std::array<T, N>& arr, const T& value_to_remove)
{
    if constexpr (N == 0)
    {
        return 0;
    }

    // 1. Use std::remove to perform the logical removal.
    //    std::remove shifts all elements *not* equal to 'value_to_remove'
    //    to the beginning of the array. The order of the remaining elements is preserved.
    //    It returns an iterator ('new_end') pointing to the element *after* //    the last non-removed element.
    auto new_end = std::remove(arr.begin(), arr.end(), value_to_remove);

    // 2. Determine the new number of valid elements.
    std::size_t new_size = std::distance(arr.begin(), new_end);

    // 3. "Clear" the space: Overwrite the remaining elements with a default value.
    //    This is equivalent to the 'erase' part of the idiom for a dynamic container.
    //    For numeric types, T{} often means 0. For class types, it calls the default constructor.
    for (auto it = new_end; it != arr.end(); ++it)
    {
        *it = T{};
    }

    // Return the new size (the count of elements NOT removed).
    return new_size;
}

template <typename T, std::size_t N, typename... Arrays>
void synchronized_remove_and_compact(
    std::array<T, N>& primaryArray,
    const T& value_to_remove,
    Arrays&... parallelArrays)
{
    if constexpr (N == 0) return;

    // 'writeIdx' tracks where the next kept element should be placed (the destination).
    std::size_t writeIdx = 0;

    // 'readIdx' tracks the element currently being inspected (the source).
    for (std::size_t readIdx = 0; readIdx < N; ++readIdx)
    {
        // Check if the current element in the primary array should be KEPT.
        if (primaryArray[readIdx] != value_to_remove)
        {
            // The element should be KEPT. We only shift if writeIdx < readIdx,
            // otherwise, it's already in the correct spot.
            if (writeIdx != readIdx)
            {
                // 1. Shift the value in the PRIMARY array.
                primaryArray[writeIdx] = primaryArray[readIdx];

                // 2. Shift the corresponding values in ALL PARALLEL arrays.
                //    This uses a C++17 fold expression to process all arguments.
                ([&]()
                    {
                        parallelArrays[writeIdx] = parallelArrays[readIdx];
                    }(), ...); // The comma operator ensures sequencing.
            }

            // Move the write pointer forward.
            writeIdx++;
        }
        // If (primaryArray[readIdx] == value_to_remove), we do nothing and
        // simply advance 'readIdx'. The 'writeIdx' stays put, and the next
        // kept element will overwrite this removed slot.
    }

    // --- Compaction (Clearing the remaining slots) ---
    // The elements from writeIdx to N-1 are duplicates of the last valid element.
    // We clear them to ensure a predictable state (like T{} or 0).
    for (std::size_t i = writeIdx; i < N; ++i)
    {
        primaryArray[i] = T{};

        ([&]()
            {
                parallelArrays[i] = typename std::decay_t<decltype(parallelArrays)>::value_type{};
            }(), ...);
    }

    // NOTE: 'writeIdx' is the new effective size of all arrays.
}


