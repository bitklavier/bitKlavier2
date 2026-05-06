// Copyright (C) 2022-2025 Dan Trueman
// SPDX-License-Identifier: GPL-3.0-or-later

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




#include "overlay.h"
#include "popup_browser.h"
#include "synth_section.h"
#include "load_save.h"
class PresetInfoCache {
  public:
    std::string getAuthor(const juce::File& preset) {
      std::string path = preset.getFullPathName().toStdString();
      if (author_cache_.count(path) == 0)
        author_cache_[path] = LoadSave::getAuthorFromFile(preset).toStdString();

      return author_cache_[path];
    }

    std::string getStyle(const juce::File& preset) {
      std::string path = preset.getFullPathName().toStdString();
      if (style_cache_.count(path) == 0)
        style_cache_[path] = LoadSave::getStyleFromFile(preset).toLowerCase().toStdString();

      return style_cache_[path];
    }

  private:
    std::map<std::string, std::string> author_cache_;
    std::map<std::string, std::string> style_cache_;
};

class PresetList : public SynthSection, public juce::TextEditor::Listener, juce::ScrollBar::Listener {
  public:
    class Listener {
      public:
        virtual ~Listener() { }

        virtual void newPresetSelected(juce::File preset) = 0;
        virtual void deleteRequested(juce::File preset) = 0;
    };

    enum Column {
      kNone,
     // kStar,
      kName,
      kStyle,
      kAuthor,
      kDate,
      kNumColumns
    };

    enum MenuOptions {
      kCancel,
      kOpenFileLocation,
      kRename,
      kDelete,
      kNumMenuOptions
    };

    static constexpr int kNumCachedRows = 50;
    static constexpr float kRowSizeHeightPercent = 0.04f;
    static constexpr float kNameWidthPercent = 0.35f;
    static constexpr float kStyleWidthPercent = 0.18f;
    static constexpr float kAuthorWidthPercent = 0.25f;
    static constexpr float kDateWidthPercent = 0.18f;
    static constexpr float kScrollSensitivity = 200.0f;

    class FileNameAscendingComparator {
      public:
        static int compareElements(juce::File first, juce::File second) {
          juce::String first_name = first.getFileNameWithoutExtension().toLowerCase();
          juce::String second_name = second.getFileNameWithoutExtension().toLowerCase();
          return first_name.compareNatural(second_name);
        }
    };

    class FileNameDescendingComparator {
      public:
        static int compareElements(juce::File first, juce::File second) {
          return FileNameAscendingComparator::compareElements(second, first);
        }
    };

    class AuthorAscendingComparator {
      public:
        AuthorAscendingComparator(PresetInfoCache* preset_cache) : cache_(preset_cache) { }

        int compareElements(juce::File first, juce::File second) {
          juce::String first_author = cache_->getAuthor(first);
          juce::String second_author = cache_->getAuthor(second);
          return first_author.compareNatural(second_author);
        }

      private:
        PresetInfoCache* cache_;
    };

    class AuthorDescendingComparator {
      public:
        AuthorDescendingComparator(PresetInfoCache* preset_cache) : cache_(preset_cache) { }

        int compareElements(juce::File first, juce::File second) {
          juce::String first_author = cache_->getAuthor(first);
          juce::String second_author = cache_->getAuthor(second);
          return -first_author.compareNatural(second_author);
        }

      private:
        PresetInfoCache* cache_;
    };

    class StyleAscendingComparator {
      public:
        StyleAscendingComparator(PresetInfoCache* preset_cache) : cache_(preset_cache) { }

        int compareElements(juce::File first, juce::File second) {
          juce::String first_style = cache_->getStyle(first);
          juce::String second_style = cache_->getStyle(second);
          return first_style.compareNatural(second_style);
        }

      private:
        PresetInfoCache* cache_;
    };

    class StyleDescendingComparator {
      public:
        StyleDescendingComparator(PresetInfoCache* preset_cache) : cache_(preset_cache) { }

        int compareElements(juce::File first, juce::File second) {
          juce::String first_style = cache_->getStyle(first);
          juce::String second_style = cache_->getStyle(second);
          return -first_style.compareNatural(second_style);
        }

      private:
        PresetInfoCache* cache_;
    };

    class FileDateAscendingComparator {
      public:
        static int compareElements(juce::File first, juce::File second) {
          juce::RelativeTime relative_time = first.getCreationTime() - second.getCreationTime();
          double days = relative_time.inDays();
          return days < 0.0 ? 1 : (days > 0.0f ? -1 : 0);
        }
    };

    class FileDateDescendingComparator {
      public:
        static int compareElements(juce::File first, juce::File second) {
          return FileDateAscendingComparator::compareElements(second, first);
        }
    };

//    class FavoriteComparator {
//      public:
//        FavoriteComparator() {
//          favorites_ = LoadSave::getFavorites();
//        }
//
//        bool isFavorite(const juce::File& file) {
//          return favorites_.count(file.getFullPathName().toStdString());
//        }
//
//        int compare(juce::File first, juce::File second) {
//          if (isFavorite(first)) {
//            if (isFavorite(second))
//              return 0;
//            return -1;
//          }
//          if (isFavorite(second))
//            return 1;
//          return 0;
//        }
//
//      private:
//        std::set<std::string> favorites_;
//    };

//    class FavoriteAscendingComparator : public FavoriteComparator {
//      public:
//        int compareElements(juce::File first, juce::File second) {
//          return compare(first, second);
//        }
//    };

//    class FavoriteDescendingComparator : public FavoriteComparator {
//      public:
//        int compareElements(juce::File first, juce::File second) {
//          return compare(second, first);
//        }
//    };

    PresetList();

    void paintBackground(juce::Graphics& g) override;
    void paintBackgroundShadow(juce::Graphics& g) override { paintTabShadow(g); }
    void resized() override;

    void setPresets(juce::Array<juce::File> presets);
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    int getRowFromPosition(float mouse_position);
    int getRowHeight() { return getHeight() * kRowSizeHeightPercent; }
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void respondToMenuCallback(int result);
    void menuClick(const juce::MouseEvent& e);
    void leftClick(const juce::MouseEvent& e);

    void mouseDown(const juce::MouseEvent& e) override;

    void textEditorReturnKeyPressed(juce::TextEditor& text_editor) override;
    void textEditorFocusLost(juce::TextEditor& text_editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override;

    void scrollBarMoved(juce::ScrollBar* scroll_bar, double range_start) override;
    void setScrollBarRange();

    void finishRename();
    void reloadPresets();
    void shiftSelectedPreset(int indices);

    void redoCache();
    void filter(juce::String filter_string, const std::set<std::string>& styles);
    int getSelectedIndex();
    int getScrollableRange();

    void initOpenGlComponents(OpenGlWrapper& open_gl) override;
    void renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) override;
    void destroyOpenGlComponents(OpenGlWrapper& open_gl) override;
    void addListener(Listener* listener) {
      listeners_.push_back(listener);
    }
    void setCurrentFolder(const juce::File& folder) {
      current_folder_ = folder;
      reloadPresets();
    }

  private:
    void viewPositionChanged();
    int getViewPosition() {
      int view_height = getHeight() - getTitleWidth();
      return std::max(0, std::min<int>(num_view_presets_ * getRowHeight() - view_height, view_position_));
    }
    void loadBrowserCache(int start_index, int end_index);
    void moveQuadToRow(OpenGlQuad& quad, int row, float y_offset);
    void sort();

    std::vector<Listener*> listeners_;
    juce::Array<juce::File> presets_;
    int num_view_presets_;
    std::vector<juce::File> filtered_presets_;
    std::set<std::string> favorites_;
    std::unique_ptr<OpenGlTextEditor> rename_editor_;
    std::unique_ptr<OpenGlScrollBar> scroll_bar_;
    juce::String filter_string_;
    std::set<std::string> filter_styles_;
    juce::File selected_preset_;
    juce::File renaming_preset_;
    juce::File current_folder_;
    int hover_preset_;
    int click_preset_;



    juce::Component browse_area_;
    int cache_position_;
    OpenGlImage rows_[kNumCachedRows];
    OpenGlQuad highlight_;
    OpenGlQuad hover_;
    float view_position_;
    Column sort_column_;
    bool sort_ascending_;
};

class PresetBrowser : public SynthSection,
                      public PresetList::Listener,
                      public juce::TextEditor::Listener,
                      public juce::KeyListener {
  public:
    static constexpr int kLeftPadding = 24;
    static constexpr int kTopPadding = 24;
    static constexpr int kMiddlePadding = 15;
    static constexpr int kNameFontHeight = 26;
    static constexpr int kAuthorFontHeight = 19;
    static constexpr int kStoreHeight = 33;
    static constexpr int kCommentsFontHeight = 15;

    class Listener {
      public:
        virtual ~Listener() { }

        virtual void newPresetSelected(juce::File preset) = 0;
        virtual void deleteRequested(juce::File preset) = 0;
        virtual void hidePresetBrowser() = 0;
    };

    PresetBrowser();
    ~PresetBrowser();

    void paintBackground(juce::Graphics& g) override;
    void paintBackgroundShadow(juce::Graphics& g) override;
    void resized() override;
    void buttonClicked(juce::Button* clicked_button) override;
    bool keyPressed(const juce::KeyPress &key, juce::Component *origin) override;
    bool keyStateChanged(bool is_key_down, juce::Component *origin) override;
    void visibilityChanged() override;

    juce::Rectangle<int> getSearchRect();
    juce::Rectangle<int> getInfoRect();

    void filterPresets();
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override;

    void newPresetSelected(juce::File preset) override {
      for (Listener* listener : listeners_)
        listener->newPresetSelected(preset);
//        loadPresetInfo();
//
//      juce::String author = author_text_->getText();
//      store_button_->setText("Get more presets by " + author);
//      bool visible = more_author_presets_.count(author.removeCharacters(" _.").toLowerCase().toStdString());
//      bool was_visible = store_button_->isVisible();
//      store_button_->setVisible(visible);
//      if (was_visible != visible)
//        setCommentsBounds();
    }

    void deleteRequested(juce::File preset) override {
      for (Listener* listener : listeners_)
        listener->deleteRequested(preset);
    }

    void loadPresets();


    void jumpToPreset(int indices);
    void loadNextPreset();
    void loadPrevPreset();
    void externalPresetLoaded(juce::File file);
    void clearExternalPreset() { external_preset_ = juce::File(); }

    void addListener(Listener* listener);

//
//    void newSelection(juce::File selection) override;
//    void allSelected() override;
//    void favoritesSelected() override;
//    void doubleClickedSelected(juce::File selection) override { }
//
  private:
    bool loadFromFile(juce::File& preset);
    void loadPresetInfo();
    void setCommentsBounds();
    void setPresetInfo(juce::File& preset);

    std::vector<Listener*> listeners_;
    std::unique_ptr<PresetList> preset_list_;
//    std::unique_ptr<OpenGlTextEditor> search_box_;
//    std::unique_ptr<SelectionList> folder_list_;
    std::shared_ptr<PlainTextComponent> preset_text_;
//    std::unique_ptr<PlainTextComponent> author_text_;
//    std::unique_ptr<OpenGlToggleButton> style_buttons_[LoadSave::kNumPresetStyles];
//    std::unique_ptr<OpenGlToggleButton> store_button_;
//
//    SaveSection* save_section_;
//    DeleteSection* delete_section_;

    std::unique_ptr<OpenGlTextEditor> comments_;
    juce::File external_preset_;
    juce::String author_;
    juce::String license_;
    std::set<std::string> more_author_presets_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowser)
};
