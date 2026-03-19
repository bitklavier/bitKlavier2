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

#include "startup.h"
#include <cstdlib>
#include <juce_core/juce_core.h>

#if JUCE_MAC && DEBUG
struct MacOSLogSilencer {
    MacOSLogSilencer() {
        setenv("OS_ACTIVITY_MODE", "disable", 1);
    }
};
static MacOSLogSilencer silence;
#endif

//#include "synth_base.h"

void Startup::doStartupChecks(/*MidiManager* midi_manager*/) {
}

bool Startup::isComputerCompatible() {
//  #if defined(__ARM_NEON__)
//  return true;
//  #else
//  return juce::SystemStats::hasSSE2() || juce::SystemStats::hasAVX2();
//  #endif
return true;
}
