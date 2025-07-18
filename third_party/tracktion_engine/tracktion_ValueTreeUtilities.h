//
// Created by Davis Polito on 1/17/25.
//

#ifndef BITKLAVIER2_TRACKTION_VALUETREEUTILITIES_H
#define BITKLAVIER2_TRACKTION_VALUETREEUTILITIES_H
/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2024
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/
#include <juce_graphics/juce_graphics.h>

#include <juce_data_structures/juce_data_structures.h>
namespace juce
{
    template <>
    struct VariantConverter<Colour>
    {
        static Colour fromVar (const var& v) { return Colour::fromString (v.toString()); }
        static var toVar (const Colour& c) { return c.toString(); }
    };
    template <>
    struct VariantConverter<juce::Point<int>>
    {
        static Point<int> fromVar (const var& v)
        {
            auto parts = StringArray::fromTokens (v.toString(), ",", "");
            if (parts.size() == 2)
            {
                int x = parts[0].getIntValue();
                int y = parts[1].getIntValue();
                return { x, y };
            }
        }
        static var toVar (const Point<int>& p) { return p.toString(); }
    };
}

namespace tracktion
{
    inline namespace engine
    {

        //==============================================================================
        /** Ensures a property is a given type which can avoid having to parse a string
    every time it is read after loading from XML.
*/
        template <typename VarType>
        inline void convertPropertyToType (juce::ValueTree& v, const juce::Identifier& id)
        {
            if (const auto* prop = v.getPropertyPointer (id))
                if (prop->isString())
                    (*const_cast<juce::var*> (prop)) = static_cast<VarType> (*prop);
        }

        /** Sets a value if it is different and returns true, otherwise simply returns false. */
        template <typename T>
        static bool setIfDifferent (T& val, T newVal) noexcept
        {
            if (val == newVal)
                return false;

            val = newVal;
            return true;
        }

        /** Returns true if the needle is found in the haystack. */
        template <typename Type>
        bool matchesAnyOf (const Type& needle, const std::initializer_list<Type>& haystack)
        {
            for (auto&& h : haystack)
                if (h == needle)
                    return true;

            return false;
        }

        /** Calls a function object on each item in an array. */
        template <typename Type, typename UnaryFunction>
        inline void forEachItem (const juce::Array<Type*>& items, const UnaryFunction& fn)
        {
            for (auto f : items)
                fn (f);
        }

        //==============================================================================
        template <typename... Others>
        void addValueTreeProperties (juce::ValueTree& v, const juce::Identifier& name, const juce::var& value, Others&&... others)
        {
            static_assert ((sizeof...(others) & 1) == 0, "The property list must be a sequence of name, value pairs");

            v.setProperty (name, value, nullptr);

            if constexpr (sizeof...(others) != 0)
                addValueTreeProperties (v, std::forward<Others> (others)...);
        }

        template <typename... Properties>
        juce::ValueTree createValueTree (const juce::Identifier& name, Properties&&... properties)
        {
            static_assert ((sizeof...(properties) & 1) == 0, "The property list must be a sequence of name, value pairs");

            juce::ValueTree v (name);
            addValueTreeProperties (v, std::forward<Properties> (properties)...);
            return v;
        }

        //==============================================================================
        template <typename ObjectType, typename CriticalSectionType = juce::DummyCriticalSection>
        class ValueTreeObjectList : public juce::ValueTree::Listener
        {
        public:
            ValueTreeObjectList (const juce::ValueTree& parentTree) : parent (parentTree)
            {
                parent.addListener (this);
            }

            ~ValueTreeObjectList() override
            {
                jassert (objects.isEmpty()); // must call freeObjects() in the subclass destructor!
            }

            inline int size() const { return objects.size(); }
            inline bool isEmpty() const noexcept { return size() == 0; }
            ObjectType* operator[] (int idx) const { return objects[idx]; }
            ObjectType* at (int idx) { return objects[idx]; }
            ObjectType** begin() { return objects.begin(); }
            ObjectType* const* begin() const { return objects.begin(); }
            ObjectType** end() { return objects.end(); }
            ObjectType* const* end() const { return objects.end(); }

            // call in the sub-class when being created
            void rebuildObjects()
            {
                jassert (objects.isEmpty()); // must only call this method once at construction

                for (const auto& v : parent)
                    if (isSuitableType (v))
                        if (auto newObject = createNewObject (v))
                            objects.add (newObject);
            }

            // call in the sub-class when being destroyed
            void freeObjects()
            {
                parent.removeListener (this);
                deleteAllObjects();
            }

            //==============================================================================
            virtual bool isSuitableType (const juce::ValueTree&) const = 0;
            virtual ObjectType* createNewObject (const juce::ValueTree&) = 0;
            virtual void deleteObject (ObjectType*) = 0;

            virtual void newObjectAdded (ObjectType*) = 0;
            virtual void objectRemoved (ObjectType*) = 0;
            virtual void objectOrderChanged() = 0;

            //==============================================================================
            void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& tree) override
            {
                if (isChildTree (tree))
                {
                    auto index = parent.indexOf (tree);
                    juce::ignoreUnused (index);
                    jassert (index >= 0);

                    if (auto* newObject = createNewObject (tree))
                    {
                        {
                            const ScopedLockType sl (arrayLock);

                            if (index == parent.getNumChildren() - 1)
                                objects.add (newObject);
                            else
                                objects.addSorted (*this, newObject);
                        }

                        newObjectAdded (newObject);
                    }
                    else
                        jassertfalse;
                }
            }

            void valueTreeChildRemoved (juce::ValueTree& exParent, juce::ValueTree& tree, int) override
            {
                if (parent == exParent && isSuitableType (tree))
                {
                    auto oldIndex = indexOf (tree);

                    if (oldIndex >= 0)
                    {
                        ObjectType* o;

                        {
                            const ScopedLockType sl (arrayLock);
                            o = objects.removeAndReturn (oldIndex);
                        }

                        objectRemoved (o);
                        deleteObject (o);
                    }
                }
            }

            void valueTreeChildOrderChanged (juce::ValueTree& tree, int, int) override
            {
                if (tree == parent)
                {
                    {
                        const ScopedLockType sl (arrayLock);
                        sortArray();
                    }

                    objectOrderChanged();
                }
            }

            void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override {}
            void valueTreeParentChanged (juce::ValueTree&) override {}

            void valueTreeRedirected (juce::ValueTree&) override { jassertfalse; } // may need to add handling if this is hit

            juce::Array<ObjectType*> objects;
            CriticalSectionType arrayLock;
            using ScopedLockType = typename CriticalSectionType::ScopedLockType;

        protected:
            juce::ValueTree parent;

            void deleteAllObjects()
            {
                const ScopedLockType sl (arrayLock);

                while (objects.size() > 0)
                    deleteObject (objects.removeAndReturn (objects.size() - 1));
            }

            bool isChildTree (juce::ValueTree& v) const
            {
                return isSuitableType (v) && v.getParent() == parent;
            }

            int indexOf (const juce::ValueTree& v) const noexcept
            {
                for (int i = 0; i < objects.size(); ++i)
                    if (objects.getUnchecked (i)->state == v)
                        return i;

                return -1;
            }

            void sortArray()
            {
                objects.sort (*this);
            }

        public:
            int compareElements (ObjectType* first, ObjectType* second) const
            {
                int index1 = parent.indexOf (first->state);
                int index2 = parent.indexOf (second->state);
                jassert (index1 >= 0 && index2 >= 0);
                return index1 - index2;
            }

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueTreeObjectList)
        };

        //==============================================================================
        template <typename ObjectType>
        struct SortedValueTreeObjectList : protected ValueTreeObjectList<ObjectType>
        {
            SortedValueTreeObjectList (const juce::ValueTree& v)
                : ValueTreeObjectList<ObjectType> (v)
            {
            }

            /** Must sort the given array. */
            virtual void sortObjects (juce::Array<ObjectType*>& objectsToBeSorted) const = 0;

            /** Should return true if the given Identifier is used to sort the tree.
                This will be called when one of the items properties changes so this should
                return true if that property means the objects should now be in a different order.
            */
            virtual bool isSortableProperty (juce::ValueTree&, const juce::Identifier&) = 0;

            /** Should return true if the objects are in a sorted order.
                This might look at a property of the objects (i.e. a start time) to determine their order.
            */
            virtual bool objectsAreSorted (const ObjectType& first, const ObjectType& second) = 0;

            //==============================================================================
            /** Returns the object for a given state. */
            const juce::Array<ObjectType*>& getSortedObjects() const
            {
                jassert (juce::MessageManager::getInstance()->currentThreadHasLockedMessageManager());
                jassert (!insidePropertyChangedMethod);

                if (setIfDifferent (needsSorting, false))
                {
                    sortedObjects = ValueTreeObjectList<ObjectType>::objects;
                    sortObjects (sortedObjects);
                }

                return sortedObjects;
            }

            //==============================================================================
            // Make sure you call the base class implementation of these methods if you override them.
            void newObjectAdded (ObjectType* o) override { triggerSortIfNeeded (o, -1); }
            void objectOrderChanged() override { triggerSort(); }

            void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& id) override
            {
                const juce::ScopedValueSetter<bool> svs (insidePropertyChangedMethod, true);

                if (!(this->isChildTree (v) && isSortableProperty (v, id)))
                    return;

                const int objectIndex = getSortedIndex (v);
                triggerSortIfNeeded (sortedObjects.getUnchecked (objectIndex), objectIndex);
            }

        private:
            mutable bool needsSorting = true;
            mutable juce::Array<ObjectType*> sortedObjects;
            bool insidePropertyChangedMethod = false;

            int getSortedIndex (const juce::ValueTree& v) const noexcept
            {
                for (int i = sortedObjects.size(); --i >= 0;)
                    if (sortedObjects.getUnchecked (i)->state == v)
                        return i;

                return -1;
            }

            void triggerSort() const
            {
                jassert (juce::MessageManager::getInstance()->currentThreadHasLockedMessageManager());
                needsSorting = true;
            }

            void triggerSortIfNeeded (ObjectType* o, int objectIndex)
            {
                if (needsSorting)
                    return;

                if (objectIndex < 0)
                    return triggerSort();

                jassert (o != nullptr);

                if (objectIndex > 0
                    && (!objectsAreSorted (*sortedObjects.getUnchecked (objectIndex - 1), *o)))
                    return triggerSort();

                if (objectIndex < (sortedObjects.size() - 1)
                    && (!objectsAreSorted (*o, *sortedObjects.getUnchecked (objectIndex + 1))))
                    return triggerSort();
            }

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SortedValueTreeObjectList)
        };

        //==============================================================================
        /** Returns the object for a given state if it exists. */
        template <typename ObjectType>
        static ObjectType* getObjectFor (const ValueTreeObjectList<ObjectType>& objectList, const juce::ValueTree& v)
        {
            for (auto* o : objectList.objects)
                if (o->state == v)
                    return o;

            return {};
        }

        //==============================================================================
        // Easy way to get one callback for all value tree change events
        struct ValueTreeAllEventListener : public juce::ValueTree::Listener
        {
            virtual void valueTreeChanged() = 0;

            //==============================================================================
            void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override { valueTreeChanged(); }
            void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override { valueTreeChanged(); }
            void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override { valueTreeChanged(); }
            void valueTreeChildOrderChanged (juce::ValueTree&, int, int) override { valueTreeChanged(); }
            void valueTreeParentChanged (juce::ValueTree&) override { valueTreeChanged(); }
            void valueTreeRedirected (juce::ValueTree&) override { valueTreeChanged(); }
        };

        template <typename Type>
        struct ValueTreeComparator
        {
        public:
            ValueTreeComparator (const juce::Identifier& attributeToSort_, bool forwards)
                : attributeToSort (attributeToSort_), direction (forwards ? 1 : -1)
            {
            }

            inline int compareElements (const juce::ValueTree& first, const juce::ValueTree& second) const
            {
                const int result = (Type (first[attributeToSort]) > Type (second[attributeToSort])) ? 1 : -1;

                return direction * result;
            }

        private:
            const juce::Identifier attributeToSort;
            const int direction;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueTreeComparator)
        };

        template <>
        inline int ValueTreeComparator<juce::String>::compareElements (const juce::ValueTree& first, const juce::ValueTree& second) const
        {
            const int result = first[attributeToSort].toString().compareNatural (second[attributeToSort].toString());

            return direction * result;
        }

        //==============================================================================
        /**
    Holds a ValueTree as a ReferenceCountedObject.

    This is somewhat obfuscated but makes it easy to transfer ValueTrees as var objects
    such as when using them as DragAndDropTarget::SourceDetails::description members.
*/
        class ReferenceCountedValueTree : public juce::ReferenceCountedObject
        {
        public:
            /** Creates a ReferenceCountedValueTree for a given ValueTree. */
            ReferenceCountedValueTree (const juce::ValueTree& treeToReference) noexcept
                : tree (treeToReference)
            {
            }

            /** Destructor. */
            ~ReferenceCountedValueTree() {}

            /** Sets the ValueTree being held. */
            void setValueTree (juce::ValueTree newTree) { tree = newTree; }

            /** Returns the ValueTree being held. */
            juce::ValueTree getValueTree() noexcept { return tree; }

            using Ptr = juce::ReferenceCountedObjectPtr<ReferenceCountedValueTree>;

            /** Provides a simple way of getting the tree from a var object which
                is a ReferencedCountedValueTree.
            */
            static juce::ValueTree getTreeFromObject (const juce::var& treeObject) noexcept
            {
                if (auto refTree = dynamic_cast<ReferenceCountedValueTree*> (treeObject.getObject()))
                    return refTree->getValueTree();

                return {};
            }

        private:
            juce::ValueTree tree;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReferenceCountedValueTree)
        };

        //==============================================================================
        /** Attempts to load a ValueTree from a file. */
        static inline juce::ValueTree loadValueTree (const juce::File& file, bool asXml)
        {
            if (asXml)
            {
                if (auto xml = juce::parseXML (file))
                    return juce::ValueTree::fromXml (*xml);
            }
            else
            {
                juce::FileInputStream is (file);

                if (is.openedOk())
                    return juce::ValueTree::readFromStream (is);
            }

            return {};
        }

        /** Attempts to load a ValueTree from a file if the outer tag matches a given name.
    This will first try to read as XML and the fall back to binary.
*/
        static inline juce::ValueTree loadValueTree (const juce::File& file, const juce::Identifier& requiredTag)
        {
            if (auto xml = juce::parseXMLIfTagMatches (file, requiredTag))
                return juce::ValueTree::fromXml (*xml);

            if (juce::FileInputStream is (file); is.openedOk())
                if (auto vt = juce::ValueTree::readFromStream (is); vt.hasType (requiredTag))
                    return vt;

            return {};
        }

        /** Saves a ValueTree to a File. */
        static inline bool saveValueTree (const juce::File& file, const juce::ValueTree& v, bool asXml)
        {
            const juce::TemporaryFile temp (file);

            {
                juce::FileOutputStream os (temp.getFile());

                if (!os.getStatus().wasOk())
                    return false;

                if (asXml)
                {
                    if (auto xml = v.createXml())
                        xml->writeTo (os);
                }
                else
                {
                    v.writeToStream (os);
                }
            }

            if (temp.getFile().existsAsFile())
                return temp.overwriteTargetFileWithTemporary();

            return false;
        }

        static inline void setPropertyIfMissing (juce::ValueTree& v, const juce::Identifier& id, const juce::var& value, juce::UndoManager* um)
        {
            if (!v.hasProperty (id))
                v.setProperty (id, value, um);
        }

        /** Creates a deep copy of another ValueTree in an undoable way, maintaining the destination's parent. */
        static inline juce::ValueTree copyValueTree (juce::ValueTree& dest, const juce::ValueTree& src, juce::UndoManager* um)
        {
            if (!dest.getParent().isValid())
            {
                dest = src.createCopy();
            }
            else
            {
                dest.copyPropertiesFrom (src, um);
                dest.removeAllChildren (um);

                for (int i = 0; i < src.getNumChildren(); ++i)
                    dest.addChild (src.getChild (i).createCopy(), i, um);
            }

            return dest;
        }

        template <typename Predicate>
        static inline void copyValueTreeProperties (juce::ValueTree& dest, const juce::ValueTree& src, juce::UndoManager* um, Predicate&& shouldCopyProperty)
        {
            for (int i = 0; i < src.getNumProperties(); ++i)
            {
                auto name = src.getPropertyName (i);

                if (shouldCopyProperty (name))
                    dest.setProperty (name, src.getProperty (name), um);
            }
        }

        static inline juce::ValueTree getChildWithPropertyRecursively (const juce::ValueTree& p,
            const juce::Identifier& propertyName,
            const juce::var& propertyValue)
        {
            const int numChildren = p.getNumChildren();

            for (int i = 0; i < numChildren; ++i)
            {
                auto c = p.getChild (i);

                if (c.getProperty (propertyName) == propertyValue)
                    return c;

                c = getChildWithPropertyRecursively (c, propertyName, propertyValue);

                if (c.isValid())
                    return c;
            }

            return {};
        }

        template <typename ValueType, typename... CachedValues>
        static void copyPropertiesToCachedValues (const juce::ValueTree& v, juce::CachedValue<ValueType>& cachedValue, CachedValues&&... cachedValues)
        {
            if (auto p = v.getPropertyPointer (cachedValue.getPropertyID()))
                cachedValue = ValueType (*p);
            else
                cachedValue.resetToDefault();

            if constexpr (sizeof...(cachedValues) != 0)
                copyPropertiesToCachedValues (v, std::forward<CachedValues> (cachedValues)...);
        }

        /** Strips out any invalid trees from the array. */
        inline juce::Array<juce::ValueTree>& removeInvalidValueTrees (juce::Array<juce::ValueTree>& trees)
        {
            for (int i = trees.size(); --i >= 0;)
                if (!trees.getReference (i).isValid())
                    trees.remove (i);

            return trees;
        }

        /** Returns a new array with any trees without the given type removed. */
        inline juce::Array<juce::ValueTree> getTreesOfType (const juce::Array<juce::ValueTree>& trees,
            const juce::Identifier& type)
        {
            juce::Array<juce::ValueTree> newTrees;

            for (const auto& v : trees)
                if (v.hasType (type))
                    newTrees.add (v);

            return newTrees;
        }

        inline void getChildTreesRecursive (juce::Array<juce::ValueTree>& result,
            const juce::ValueTree& tree)
        {
            for (auto child : tree)
            {
                result.add (child);
                getChildTreesRecursive (result, child);
            }
        }

        inline void renamePropertyRecursive (juce::ValueTree& tree,
            const juce::Identifier& oldName,
            const juce::Identifier& newName,
            juce::UndoManager* um)
        {
            if (auto oldProp = tree.getPropertyPointer (oldName))
            {
                tree.setProperty (newName, *oldProp, um);
                tree.removeProperty (oldName, um);
            }

            for (auto child : tree)
                renamePropertyRecursive (child, oldName, newName, um);
        }

    }
} // namespace tracktion { inline namespace engine
#endif //BITKLAVIER2_TRACKTION_VALUETREEUTILITIES_H
