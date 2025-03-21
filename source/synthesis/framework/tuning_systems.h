//
// Created by Dan Trueman on 10/22/24.
//


#ifndef BITKLAVIER2_TUNING_SYSTEMS_H
#define BITKLAVIER2_TUNING_SYSTEMS_H

#pragma once

//using namespace juce;

// original bK tunings
const juce::Array<float> tEqualTuning               = juce::Array<float>( {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.} );
const juce::Array<float> tJustTuning                = juce::Array<float>({ 0., .117313, .039101, .156414, -.13686, -.019547, -.174873, .019547, .136864, -.15641, -.311745, -.11731 });
const juce::Array<float> tPartialTuning             = juce::Array<float>({ 0., .117313, .039101, -.331291, -.13686, -.019547, -.486824, .019547, .405273, -.15641, -.311745, -.506371 });
const juce::Array<float> tDuodeneTuning             = juce::Array<float>({ 0., .117313, .039101, .156414, -.13686, -.019547, -.097763, .019547, .136864, -.15641, -.039101, -.11731 });
const juce::Array<float> tOtonalTuning              = juce::Array<float>({ 0., .049553, .039101, -.02872, -.13686, -.292191, -.486824, .019547, .405273, .058647, -.311745, -.11731 } );
const juce::Array<float> tUtonalTuning              = juce::Array<float>({ 0., .117313, .311745, .156414, -.405273, -.019547, .486824, .292191, .136864, .024847, -.039101, -.049553 } );

//historical temperaments
const juce::Array<float> tPythagorean               = juce::Array<float>({ 0., 0.13685, 0.0391, -0.05865, 0.0782, -0.01955, 0.1173, 0.01955, 0.1564, 0.05865, -0.0391, 0.09775} );
const juce::Array<float> tGrammateus                = juce::Array<float>({ 0., 0.01955, 0.0391, 0.05865, 0.0782, -0.01955, 0, 0.01955, 0.0391, 0.05865, 0.0782, 0.09775 });
const juce::Array<float> tKirnbergerII              = juce::Array<float>({ 0., -0.0977871, 0.0391, -0.0586871, -0.1369, -0.0195871, -0.0978, 0.01955, -0.0782371, -0.0489, -0.0391371, -0.11735 });
const juce::Array<float> tKirnbergerIII             = juce::Array<float>({ 0., -0.0977871, -0.06845, -0.0586871, -0.1369, -0.0195871, -0.0978, -0.034225, -0.0782371, -0.102675, -0.0391371, -0.11735 });
const juce::Array<float> tWerkmeisterIII           = juce::Array<float>({0, -0.0977871, -0.06845, -0.0586871, -0.083125, -0.0195871, -0.0978, -0.034225, -0.0782371, -0.102675, -0.0391371, -0.063575 });
const juce::Array<float> tQuarterCommaMeantone     = juce::Array<float>({0, -0.239575, -0.06845, 0.122175, -0.1369, 0.053725, -0.20535, -0.034225, -0.2738, -0.102675, 0.08795, -0.171125 });
const juce::Array<float> tSplitWolfQCMeantone      = juce::Array<float>({0, -0.239575, -0.06845, -0.092925, -0.1369, 0.053725, -0.20535, -0.034225, -0.2738, -0.102675, 0.08795, -0.171125 });
const juce::Array<float> tTransposingQCMeantone    = juce::Array<float>({0, -0.239575, -0.06845, 0.122175, -0.1369, 0.053725, -0.20535, -0.034225, 0.1564, -0.102675, 0.08795, -0.171125 });
const juce::Array<float> tCorrette                 = juce::Array<float>({0, -0.239575, -0.06845, -0.0212249, -0.1369, 0.0509674, -0.20535, -0.034225, -0.23795, -0.102675, 0.0148713, -0.171125 });
const juce::Array<float> tRameau                   = juce::Array<float>({0, -0.132025, -0.06845, -0.0301876, -0.1369, 0.0537248, -0.151575, -0.034225, -0.148325, -0.102675, 0.0879498, -0.171125 });
const juce::Array<float> tMarpourg                 = juce::Array<float>({0, -0.14995, -0.06845, -0.0391498, -0.1369, 0.0716504, -0.20535, -0.034225, -0.0945499, -0.102675, 0.0162503, -0.171125 });
const juce::Array<float> tEggarsEnglishOrd         = juce::Array<float>({0, -0.1858, -0.06845, -0.092925, -0.1369, 0.053725, -0.178463, -0.034225, -0.16625, -0.102675, -0.0196, -0.171125 });
const juce::Array<float> tThirdCommaMeantone       = juce::Array<float>({0, -0.365049, -0.1043, 0.175951, -0.2086, 0.0716508, -0.3129, -0.0521499, -0.417199, -0.15645, 0.123801, -0.26075 });
const juce::Array<float> tDAlembertRousseau        = juce::Array<float>({0, -0.132025, -0.06845, -0.0943036, -0.1369, -0.0221113, -0.13365, -0.034225, -0.1304, -0.102675, -0.0582074, -0.135275 });
const juce::Array<float> tKellner                  = juce::Array<float>({0, -0.09775, -0.05474, -0.05865, -0.10948, -0.01955, -0.1173, -0.02737, -0.0782, -0.08211, -0.0391, -0.08993 });
const juce::Array<float> tVallotti                 = juce::Array<float>({0, -0.0586504, -0.0391002, -0.0195504, -0.0782003, 0.0195496, -0.0782004, -0.0195501, -0.0391004, -0.0586502, 0, -0.0977504 });
const juce::Array<float> tYoungII                  = juce::Array<float>({0, -0.0977505, -0.0391002, -0.0586505, -0.0782003, -0.0195505, -0.1173, -0.0195501, -0.0782005, -0.0586502, -0.0391005, -0.0977504 });
const juce::Array<float> tSixthCommaMeantone       = juce::Array<float>({0, -0.136851, -0.0391002, 0.0586493, -0.0782003, 0.0195491, -0.1173, -0.0195501, -0.156401, -0.0586502, 0.0390992, -0.0977504 });
const juce::Array<float> tBachBarnes               = juce::Array<float>({0, -0.0586504, -0.0391002, -0.0195504, -0.0782003, 0.0195496, -0.0782004, -0.0195501, -0.0391004, -0.0586502, 0, -0.0586503});
const juce::Array<float> tNeidhardt                = juce::Array<float>({0, -0.0391, -0.0391002, -0.0195499, -0.0586502, -0.0195498, -0.0391001, -0.0195501, -0.0390999, -0.0586502, -0.0195498, -0.0391002 });
const juce::Array<float> tBachLehman               = juce::Array<float>({0, -0.0195503, -0.0391002, -0.0195502, -0.0782003, 0.0180461, -0.0391003, -0.0195501, -0.0195502, -0.0586502, -0.0195501, -0.0586503 });
const juce::Array<float> tBachODonnell             = juce::Array<float>({0, -0.0391002, -0.0391002, -0.0391001, -0.0391002, -0.01955, -0.0391003, -0.0195501, -0.0391002, -0.0586502, -0.0391, -0.0586503 });
const juce::Array<float> tBachHill                 = juce::Array<float>({0, -0.0436113, -0.0330845, -0.0225575, -0.0661691, 0.0165425, -0.0451152, -0.0165423, -0.0421075, -0.0496268, -0.00300749, -0.0466191 });
const juce::Array<float> tBachSwich                = juce::Array<float>({0, -0.0703799, -0.05474, -0.0182465, -0.10948, 0.0273702, -0.0899299, -0.02737, -0.0443132, -0.08211, 0.00782023, -0.08993 });
const juce::Array<float> tLambert                  = juce::Array<float>({0, -0.0642355, -0.0279285, -0.0251355, -0.055857, 0.0139645, -0.0837855, -0.0139642, -0.0446855, -0.0418928, -0.00558551, -0.0698213 });
const juce::Array<float> tEighthCommaWT            = juce::Array<float>({0, -0.0391, -0.01955, 0, -0.0391, 0.009775, -0.05865, -0.009775, -0.01955, -0.029325, 0.01955, -0.048875 });
const juce::Array<float> tPinnockModern            = juce::Array<float>({0, -0.0390998, -0.01955, 0, -0.0391, 0.0195503, -0.0390998, -0.009775, -0.0195498, -0.029325, 0, -0.0390999 });

const juce::Array<float> tCommonJust              = juce::Array<float>({0, 0.11731285269777758, 0.039100017307748376, 0.15641287000552553, -0.13686286135165177, -0.019550008653875465, -0.09776284404390367, 0.019550008653873192, 0.13686286135165232, -0.15641287000552553, 0.175962878659401, -0.117312852697778});
const juce::Array<float> tSymmetric               = juce::Array<float>({0, 0.11731285269777758, 0.039100017307748376, 0.15641287000552553, -0.13686286135165177, -0.019550008653875465, 0., 0.019550008653873192, 0.13686286135165232, -0.15641287000552553, -0.03910001730774866, -0.117312852697778 });
const juce::Array<float> tWellTunedPiano          = juce::Array<float>({0, 0.7664590993067458, 0.039100017307748376, -0.603931861963627, 0.7078090733451233, -0.564831844655879, 0.746909090652872, 0.019550008653873192, -0.6234818706175008, 0.6882590646912502, -0.5843818533097533, 0.727359081999 });
const juce::Array<float> tHarrisonStrict          = juce::Array<float>({0, -0.3703909612703742, 0.039100017307748376, -0.05865002596162355, -0.13686286135165177, -0.019550008653875465, -0.3899409699242483, 0.019550008653873192, -0.07820003461549846, -0.15641287000552553, -0.03910001730774866, -0.117312852697778 });

#endif //BITKLAVIER2_TUNING_SYSTEMS_H

