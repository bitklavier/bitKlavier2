# bitKlavier 2
the digital prepared piano

bitKlavier2 uses the [PampleJuce](docs/PampleJuceREADME.md) CMake framework 
to avoid the the use of the Projucer and make it easier to include external 
libraries

## External Libraries
External libraries can be found in both the [modules](modules) 
and [third_party](third_party) folders. 
### modules
Modules is intended for use with
JUCE specific modules which have their proprietary include structure set by the
JUCE standard.Information about how to include these in cmake can be found 
[here](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md#juce_add_module)
Information about the format for creating a juce module can be found [here](https://github.com/juce-framework/JUCE/blob/master/docs/JUCE%20Module%20Format.md)
build structure 


 - information about how this works can be
   found in this [melatonin_audio_sparklines blog post]([here](https://melatonin.dev/blog/audio-sparklines/))
-  [melatonin_audio_sparklines](https://github.com/sudara/melatonin_audio_sparklines)
  - allows us to print audio to the console in debug mode using  `melatonin::printSparkline(myAudioBlock)`


### third_party

this folder is third party libraries that are included without the `juce_add_submodule` command.
The usual way these libraries are added is with
[add_subdirectory](https://cmake.org/cmake/help/latest/command/add_subdirectory.html) 
in our [CMakeLists.txt](CMakeLists.txt) file. `add_subdirectory` works by searching 
the directory listed for its own `CMakeLists.txt` to determine how to build the library.

- juce-toys
  - using the juce_lldb_xcode.py file to make juce::string and juce::component
    display more usable debug information. Installation instructions can be found at
    the top of the [file](modules/juce-toys/juce_lldb_xcode.py)
  - also [value_tree_debugger](source/common/valuetree_utils/value_tree_debugger.h)
  has been removed form the module and put in our source
- sfzq 
  - sound fonts (unimplemented)
- [melatonin_inspector](https://github.com/sudara/melatonin_inspector)
  - allows us to view information and move around `juce::Components` in our UI
  - *NOTE: the features of this module do not all work due to the use of
    openGLRendering* 
- [tracktion_engine](https://github.com/Tracktion/tracktion_engine)
  - we were previouisly linking against this whole library as a submodule
  but after various headaches from using it [davispolito](https://github.com/davispolito) 
  decided to just grab the utility files we wanted and put them here
- chowdsp
  - more to come

https://git-lfs.com 

### Parameter Listeners 

if you are trying to debug parameter listeners you should put a breakpoint in [chowdsp_ParameterListeners.cpp](third_party/chowdsp_utils/modules/plugin/chowdsp_plugin_state/Backend/chowdsp_ParameterListeners.cpp)
there are several functions to study here but most often you will be interested in 
``` 
void ParameterListeners::parameterValueChanged (int paramIndex, float newValue)
{
    if (! juce::MessageManager::getInstance()->isThisTheMessageThread())
        return; // this will be handled by the timer callback!

    auto index = static_cast<size_t> (paramIndex);
    auto& paramInfo = paramInfoList[index];
    paramInfo.value = newValue;
    audioThreadBroadcastQueue.try_enqueue ([this, i = index]
                                           { callAudioThreadBroadcaster (i); });
    callMessageThreadBroadcaster (index);
}
```

this function should get hit everytime any audio parameter changes from the gui





### A Note on Code Signing 5/6/2025 - Davis Polito
since apple code signing is one of the worst processes ever invented I have endeavored 
to enlist the help of those far more intelligent than myself to integrate github actions 
into our build pipeline. Github Actions we are able to set up a CI/CD pipeline to 
build, test and sign our applications when they are pushed to github. To allow for the signing process
to run remotely we must implement [Github Secret](https://docs.github.com/en/actions/security-for-github-actions/security-guides/using-secrets-in-github-actions).
This allows us to store protected variables in github that are not exposed to viewers of the public repo.
The secrets and steps to generate them can be found on [this](https://melatonin.dev/blog/how-to-code-sign-and-notarize-macos-audio-plugins-in-ci/#troubleshooting-code-signing-and-notarytool-issues)  
blog post from the author of the PampleJuce template we use as well as the [macos keychain action](https://github.com/sudara/basic-macos-keychain-action)
which is integrated into the template. I will list a few keypoints from the article and from debugging the issues seen in order to troubleshoot problems that may be faced after my departure.
1. Generate .p12 files from the Xcode Certificates menu.  
  - in theory we can generate the .p12 file from Keychain Access but I encountered a bug which would cause `productsign` / `productbuild` --sign to hang with no output when using a INSTALLER cert generated from Keychain Access
  - the Certifications can be found in Xcode -> settings->account -> manager certificates. You can then export them with a write click. 
  - after exporting a certificate you must convert it into a base64 string for upload to github actions. the command to do so is 
    `base64 -i <p12file.p12> | pbcopy` this will copy the string to your clipboard which you can then paste into github actions
    - The secrets/variables can be found in github under the organization/team settings page under the Secrets and Variables -> Actions tab 
    - The secrets and variables that will need to change are the `DEVELOPER_ID_APP_CERT`, `DEVELOPER_ID_APP_PASSWORD`, `DEVELOPER_ID_INSTALLER_CERT`, and `DEVELOPER_INSTALLER_PASSWORD` all  other variables should remain the same as they do not expire
    2. There is a 'known' bug in our codebase where using the distribution template supplied by pamplejuce results in hanging on the productbuild step. I hope to remedy this soon
       - it is easier to debug simple issues with certs using the [test repo](https://github.com/bitklavier/test) I created which is a very simple app example that doesn't require juce or really do anything. It just creates an executable .app and signs it
       - for this reason i use `productbuild --synthesize` to generate a default template and sign that rather than having a nice distribution.xml file pre-built
    3. when failing at the notarization step we can use `xcrun notarytool log` offline to pull the log file from apple 
       - a failed staple will output an error in github actions such as 
       ```
       Current status: Invalid.............Processing complete
        id: 437befe-2e52-401f-b17a-91b2de977cb5
        status: Invalid
        ```
       - we can then query apple for the log output of that process on our own computer through 
        ```
       xcrun notarytool log 437befe-2e52-401f-b17a-91b2de977cb5 --keychain-profile dtrueman@princeton.edu > log.json
       ```
       - this function assumes that you are running it from a computer that is signed into the apple account in question i.e. you are on Daniel Trueman's computer in the case of this project  
                 
       - if your name is not daniel trueman and you are not someone using daniel trueman's computer. Perhaps your name is myra norton.
         you will need some information from daniel trueman. Namely the app specific password we use for signing and the team-id to sign with -- the information in parentheses of the Installer/Application strings.
         With that information you can query apple for the log information using this command. 
         ```
         xcrun notarytool log 437befe-2e52-401f-b17a-91b2de977cb5 --apple-id "dtrueman@princeton.edu"  
          --team-id "YOURTEAMID" --password "abcd-efgh-ijkl-mnop" > log.json
         ```
For more info checkout [this](https://forum.juce.com/t/pkgbuild-and-productbuild-a-tutorial-pamplejuce-example/64977) post by sudara or his blog [https://melatonin.dev/blog/](https://melatonin.dev/blog/) for more info
There is also a [manual](https://melatonin.dev/manuals/pamplejuce/) for the general template that might feature useful information on these topics.