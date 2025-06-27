# Creating and Deleting
# Creating
There are two ways to do this:
- Press the one of the hot keys
- Right click anywhere on the screen and select the preparation you want from the from the popup

Either one of these actions results in the a preparation popping up on your screen. Let's first walk through how this happens at a high level then we'll dig into some details in a function call trace.
We will use the direct preparation as an example for simplicity.

## Creation Overview
1. ConstructionSite registers a key press or handles the popup selection (however you decide to create the preparation)
2. We add a direct preparation ValueTree to the PREPARATIONS ValueTree
3. We create and set up the AudioProcessor for the direct preparation
4. We add that processor to the AudioProcessorGraph
5. We create a PreparationSection for the direct preparation and add it to the list of things that openGL renders
6. We add the PreparationSection to the ConstructionSite's list of components so that we can continue to work with it
7. OpenGL draws the preparation that is now properly hooked up and ready to use

## Creation Function Call Trace

1. When we press the 'd' key, [ConstructionSite::keyPressed()](../source/interface/sections/ConstructionSite.cpp#L185) gets called. When we right-click on the ConstructionSite and select 'Direct' from the popup, [ConstructionSite::handlePluginPopup()](../source/interface/sections/ConstructionSite.cpp) gets called
    
    - In both functions, we create a [ValueTree](#the-valuetree) with ID "PREPARATION" and set the properties (type, width, height, and x & y positions)
2. From either the keyPressed() or the handlePluginPopup() function, we call [prep_list.appendChild()](../source/interface/Preparations/PreparationList.h)

   - `prep_list` is the [PreparationList](#the-preparationlist) object in ConstructionSite that keeps track of all the preparations, and its appendChild() function looks like this
   ```
   void appendChild (const juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        this->parent.appendChild(child,undoManager);
    }
   ```
   - `parent` is the PREPARATIONS ValueTree, and `child` is the PREPARATION (note there is no 'S' at the end) ValueTree that we have created for our direct preparation. I won't go down the rabbit hole of functions that gets called from this... Just know that all we had to do was write this one line of code and JUCE will take care of making PREPARATION a child of PREPARATIONS in our ValueTree.
3. When this preparation gets added to the PREPARATIONS ValueTree, the [valueTreeChildAdded()](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) function from the [ValueTreeObjectList](#the-valuetreeobjectlist) is listening...
4. This function sees that we're adding a preparation to the ValueTree so it calls [createNewObject()](../source/interface/Preparations/PreparationList.cpp) which has been implemented by our [PreparationList](#the-preparationlist) object.

      - createNewObject() is where we finally create the AudioProcessor for the direct preparation and set it up with the sample rate and buffer size
5. From within createNewObject(), we call SynthBase's [synth.addProcessor()](../source/synthesis/synth_base.cpp), which calls [engine_->addNode()](../source/synthesis/sound_engine/sound_engine.h)

   - This is where we create a node for our direct AudioProcessor and add it to the [AudioProcessorGraph](#the-audioprocessorgraph)
6. Back in step 3, we said that [valueTreeChildAdded()](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) was listening to changes in our ValueTree and called createNewObject() when it saw that we had added our direct preparation to the ValueTree. After doing that, it calls [newObjectAdded()](../source/interface/Preparations/PreparationList.cpp), which is another function implemented by our PreparationList object

   - newObjectAdded() says "Hey, who's [listening](#listeners)? Who needs to know if we've added something?" And the ConstructionSite says "Ooo! Me!"
7. From newObjectAdded(), we call the ConstructionSite's [moduleAdded()](../source/interface/sections/ConstructionSite.cpp) function.
8. From moduleAdded(), we call [addSubSection()](../source/interface/sections/synth_section.cpp)

   - This function lives in SynthSection (which ConstructionSite inherits from). It creates a PreparationSection (the GUI component) for our preparation and adds a pointer to it to `sub_sections_` (the vector of SynthSection pointers) that [renderOpenGlComponents()](../source/interface/sections/synth_section.cpp) draws
9. Finally, in [moduleAdded()](../source/interface/sections/ConstructionSite.cpp), after we create the PreparationSection, we add a unique pointer to the PreparationSection to `plugin_components` - the ConstructionSite's vector of unique pointers to PreparationSections. If we didn't do this, we'd lose the subsection once the function finished running and everything will crash.
10. **ALL DONE!** The direct preparation shows up on our screen, and we've added it to all the important places:

- the ValueTree
- the AudioProcessorGraph
- [`objects`](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h): the array of PluginInstanceWrapper pointers in the ValueTreeObjectList
- [`sub_sections_`](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h): the vector of SynthSection pointers in SynthSection
- [`plugin_components`](../source/interface/sections/ConstructionSite.h): the vector of unique pointers to PreparationSections in ConstructionSite

**NICE!**
## Deleting
There are 3 ways a preparation could be deleted
- press the 'delete' key on your keyboard with the preparation selected
- press 'CMD + z' on your keyboard
- click Edit > Undo

Let's first walk through how this happens at a high level then we'll dig into some details in a function call trace.

## Deletion Overview
1. ApplicationCommandHandler registers the selection of Edit > Undo, ApplicationCommandManager registers when you press 'CMD + z' or 'delete' on your keyboard.
2. We remove the direct preparation from the ValueTree
3. We remove it from `objects` - the ValueTreeObjectList's array of PluginInstanceWrapper objects
4. We find the preparation's PreparationSection in `plugin_components` and remove its listener
5. We remove the PreparationSection from `sub_sections_` - the list of things that openGL renders (being careful to do this on the OpenGL thread)
6. We delete the OpenGL components of the PreparationSection and remove the PreparationSection from the ConstructionSite's `plugin_components`
7. We remove the preparation's AudioProcessor from the AudioProcessorGraph
8. Our direct preparation disappears from the screen and is removed from the audio processing.

## Deletion Function Call Trace
Before tracing, let's think about everything we had to do to create the preparation. To make sure we clean up all of the moving pieces, we need to make sure that we: 1) remove the parameter from the ValueTree, 2) remove the parameter from the AudioProcessorGraph, 3) remove the parameter from `objects`, 4) remove the parameter from `sub_sections_`, and 5) remove the parameter from `plugin_components`. Not only do we need to remove it from all of these things, but we need to make sure that it is deleted as well. Starting from the top...

1. When we select Edit > Undo, [ApplicationCommandHandler::perform()](../source/common/ApplicationCommandHandler.cpp) will call
   ```c++
   parent->getUndoManager()->undo();
   ```
   This gets the [UndoManager](#the-undomanager) from `parent`, which is a pointer to the SynthGuiInterface, and calls the UndoManager's undo() function
   
   When we press 'CMD + z' [SynthGuiInterface::perform()](../source/common/synth_gui_interface.cpp) will simply call `getUndoManager()->undo()`.
2. [UndoManager::undo()](../JUCE/modules/juce_data_structures/undomanager/juce_UndoManager.cpp) is implemented by JUCE and takes care of removing our preparation from the ValueTree. 
   - **We've removed from the ValueTree - 1 of 5 removal steps accomplished**
3. When our preparation is removed from the ValueTree, the [valueTreeChildRemoved()](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h) function from the [ValueTreeObjectList](#the-valuetreeobjectlist) is listening...
   - In this function, we call
      ```c++
      o = objects.removeAndReturn (oldIndex);
      ```
      where `objects` is an array of pointers to [PluginInstanceWrapper](#the-plugininstancewrapper) objects and removeAndReturn() is a juce_Array function that returns a pointer to the preparation (PluginInstanceWrapper) that we're removing.
   - **We've removed from `objects` - 2 of 5 removal steps accomplished**
4. Then it calls [objectRemoved()](../source/interface/Preparations/PreparationList.cpp) which has been implemented by our [PreparationList](#the-preparationlist) object.
   - objectRemoved() says "Hey, who's listening? Who needs to know if we've removed something?" And the ConstructionSite says "Ooo! Me!"
5. From objectRemoved(), we call the ConstructionSite's [removeModule()](../source/interface/sections/ConstructionSite.cpp) function.

   - Here, we go through `plugin_components`, find the PreparationSection with the same ID as the PluginInstanceWrapper that we’re removing.
   - Now that we have the PreparationSection, we have to remove the preparation's listener with the following code
   ```c++
    preparationSelector.getLassoSelection().removeChangeListener (plugin_components[index].get());
   ```
   - Next, we need to remove the PreparationSection from both `sub_sections_` and `plugin_components`.
6. From removeModule(), we call [removeSubSection()](../source/interface/sections/synth_section.cpp) to remove the PreparationSection from `sub_sections_`.

   - This function lives in SynthSection (which ConstructionSite inherits from) and removes the PreparationSection from `sub_sections_` (the vector of SynthSection pointers that [renderOpenGlComponents()](../source/interface/sections/synth_section.cpp) draws) by calling 
   ```c++
   sub_sections_.erase(location);
   ```
   - But this isn't 100% right yet. Since OpenGL uses `sub_sections_` to render things, we need to make sure that manipulating `sub_sections_` happens on the OpenGL thread, which looks like this
   ```c++
    {
        juce::ScopedLock lock(open_gl_critical_section_);
        removeSubSection (plugin_components[index].get());
    }
   ```
   - If you want to know more about what this is doing, check out the section I've written about [locking](#locking).
   - **We've removed from `sub_sections_` - 3 of 5 removal steps accomplished**
7. Now we go back to the ConstructionSite's [removeModule()](../source/interface/sections/ConstructionSite.cpp) and remove our parameter's PreparationSection from `plugin_components`.

   - If simply try to remove the pointer from the vector, we run into an error. Before we can remove the PreparationSection pointer from `plugin_component`, we need to destroy the OpenGL components (the buffer, shader, and texture) of our PreparationSection like so
      ```c++
      plugin_components[index]->destroyOpenGlComponents (open_gl);     
      plugin_components.erase(plugin_components.begin()+index);
      ```
   - But wait! Deleting OpenGL components should be happening on the OpenGL thread, so why aren't we locking like we did in step 7? It's already taken care of for us in the destroyOpenGlComponents() function.
   - **We've removed from `plugin_components` - 4 of 5 removal steps accomplished**
   - Now, if we build and run, create a direct preparation, and click Edit > Undo, it works! The direct preparation disappears! But we have one removal step left: Removing our preparation from the AudioProcessorGraph.
8. Back in step 4 of the deletion process, we were in the PreparationList's [objectRemoved()](../source/interface/Preparations/PreparationList.cpp) function. After it calls the ConstructionSite's removeModule() function, we call SynthBase's [synth.removeProcessor()](../source/synthesis/synth_base.cpp), which calls [engine_->removeNode()](../source/synthesis/sound_engine/sound_engine.h)
   - removeNode() is a JUCE function that takes care of removing the node for our preparation from the [AudioProcessorGraph](#the-audioprocessorgraph)
   - **We've removed from the AudioProcessorGraph - 5 of 5 removal steps accomplished**
9. We might think we're done, but we have one more bug to take care of. If we create a direct preparation and undo, it appears to work just fine. If we create two direct preparation and undo, both of our direct preparations disappear. To fix this, we modify the PreparationList's [appendChild()](../source/interface/Preparations/PreparationList.h) function like so
   ```c++
    void appendChild (const juce::ValueTree& child, juce::UndoManager* undoManager)
    {
        undoManager->beginNewTransaction();
        this->parent.appendChild(child,undoManager);
    }
   ```
   - This just tells the UndoManager that appending a Child is something that can be undone
10. **ALL DONE!** Now when we undo, the direct preparation disappears our screen, and we've removed it from all the important places:

- the ValueTree
- the AudioProcessorGraph
- [`objects`](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h): the array of PluginInstanceWrapper pointers in the ValueTreeObjectList
- [`sub_sections_`](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h): the vector of SynthSection pointers in SynthSection
- [`plugin_components`](../source/interface/sections/ConstructionSite.h): the vector of unique pointers to PreparationSections in ConstructionSite

**NICE!**
## Important Pieces of The Puzzle
If you read through function call traces and you had a hard time keeping track of all the moving pieces... me too. So, here's some more information about the important parts of the puzzle, namely:

- [The ValueTree](#the-valuetree)
- [The ValueTreeObjectList](#the-valuetreeobjectlist)
- [The PluginInstanceWrapper](#the-plugininstancewrapper)
- [The PreparationList](#the-preparationlist)
- [The UndoManager](#the-undomanager)
- [The AudioProcessorGraph](#the-audioprocessorgraph)
- [Locking](#locking)
- [Listeners](#listeners)
- Sections?

### The ValueTree
The ValueTree is implemented by JUCE and provides a whole lot of useful functionality for us. The creators explain it best, so you can read more about it [here](https://juce.com/tutorials/tutorial_value_tree/). A ValueTree is made up of other ValueTrees, and our main ValueTree gets created in [SynthBase](../source/synthesis/synth_base.cpp), which is the start of our backend stuff. There, we create a ValueTree with ID "GALLERY" then create another ValueTree with ID "PIANO". We make the PIANO ValueTree a child of the GALLERY ValueTree. Then we make three more ValueTrees that are all children of the "PIANO" ValueTree. Once we're done setting it up, it looks like this
``` xml
<GALLERY>
   <PIANO>
      <PREPARATIONS/>
      <CONNECTIONS/>
      <MODCONNECTIONS/>
   </PIANO>
</GALLERY>
```
For creating and deleting preparations, you just need to concern yourself with the PREPARATIONS ValueTree. This is because anytime you want to create a preparation, you need to create a ValueTree for the preparation and add it as a child of the PREPARATIONS ValueTree like so
``` xml
<GALLERY>
   <PIANO>
      <PREPARATIONS>
         <PREPARATION type=”” width=”” height=”” x=”” y=”” … numOuts=”” />
      </PREPARATIONS> 
      <CONNECTIONS/>
      <MODCONNECTIONS/>
   </PIANO>
</GALLERY>
```
### The ValueTreeObjectList
The ValueTreeObjectList is a class created by Tracktion that facilitates responding to changes in the Value Tree. You can read the documentation [here](https://tracktion.github.io/tracktion_engine/classtracktion_1_1engine_1_1ValueTreeObjectList.html). In general, the ValueTreeObjectList listens to the value tree and implements any necessary functions. This class has a few virtual void functions, which means we have to write our own versions of these functions. Such functions include newObjectAdded(), objectRemoved(), etc. We implement these functions in the [PreparationList](#the-preparationlist) class.

### The PluginInstanceWrapper
The [PluginInstanceWrapper](../source/interface/Preparations/PreparationList.h) is a class that we have created to bundle all of the important information for an instance of a plugin (a preparation in our case). The member variables are:

- a pointer to a JUCE AudioProcessor
- a node ID for a JUCE AudioProcessorGraph
- a JUCE ValueTree

You can simply think of this as the parameter (with all of its important information)

### The PreparationList
The [PreparationList](../source/interface/Preparations/PreparationList.h) is a class that we have created to keep track of all of the preparations. It has an array of [PluginInstanceWrappers](#the-plugininstancewrapper) called `objects` that holds all of the preparations. It also has a value tree called `parent`, which is the PREPARATIONS value tree. Any preparation that we add will be a child of PREPARATIONS.

Whenever the value tree is changed, the PreparationList will notice (it's always [listening](#listeners)...) and it will update any variables and run any functions that it needs to.

How is our PreparationList always listening? And why don't I see `objects` or `parent` in the PreparationList files? This is because PreparationList inherits from [tracktion::engine::ValueTreeObjectList\<PluginInstanceWrapper>](../third_party/tracktion_engine/tracktion_ValueTreeUtilities.h), which is a third-party class called the [ValueTreeObjectList](#the-valuetreeobjectlist). Our PreparationList is like an extension of the [ValueTreeObjectList](#the-valuetreeobjectlist) that makes their awesome functions work on our specific project. We just have to implement our versions of certain ValueTreeObjectList functions, such as newObjectAdded() and objectRemoved() in PreparationList to make sure everything works correctly.

The PreparationList object is created in SynthBase and is passed into the ConstructionSite as part of the SynthGuiData. The PreparationList only lives in the ConstructionSite (no where else) as `prep_list`.

### The UndoManager
One of the convenient things about using JUCE's value tree is that we get access to undo/redo functions implemented by their UndoManager. You can read the documentation [here](https://docs.juce.com/master/classUndoManager.html).

Like our top-level value tree, the UndoManager gets created in [SynthBase](../source/synthesis/synth_base.cpp), which is the start of our backend stuff. This UndoManager gets passed around all over the place. We first use it in ConstructionSite, but how does it get there? Buckle up... ConstructionSite receives it as an argument when it is constructed in MainSection. Similarly, MainSection receives it as an argument when it is constructed in FullInterface. FullInterface is constructed in SynthGuiInterface with a single argument: `SynthGuiData* synth_data`. The UndoManager is lumped into that SynthGuiData object, which holds the SynthBase where the UndoManager was created.

### The AudioProcessorGraph
### Locking
### Listeners
### Sections
