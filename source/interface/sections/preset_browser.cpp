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

#include "preset_browser.h"

#include "skin.h"
#include "fonts.h"
#include "paths.h"
#include "synth_gui_interface.h"
#include "open_gl_component.h"
#include "open_gl_image.h"
#include "synth_section.h"

namespace {
  template<class Comparator>
  void sortFileArray(juce::Array<juce::File>& file_array) {
    Comparator comparator;
    file_array.sort(comparator, true);
  }

template<class Comparator>
  void sortFileArrayWithCache(juce::Array<juce::File>& file_array, PresetInfoCache* cache) {
    Comparator comparator(cache);
    file_array.sort(comparator, true);
  }

  const std::string kPresetStoreUrl = "";

  class FileNameFilter : public juce::TextEditor::InputFilter {
    public:
      FileNameFilter() : juce::TextEditor::InputFilter() { }

      juce::String filterNewText(juce::TextEditor& editor, const juce::String& new_input) override {
        return new_input.removeCharacters("<>?*/|\\[]\":");
      }
  };
}

PresetList::PresetList() : SynthSection("Preset List"),
    num_view_presets_(0), hover_preset_(-1), click_preset_(-1), cache_position_(0),
    highlight_(Shaders::kColorFragment), hover_(Shaders::kColorFragment),
    view_position_(0), sort_ascending_(true) {
  addAndMakeVisible(browse_area_);
  browse_area_.setInterceptsMouseClicks(false, false);
  highlight_.setTargetComponent(&browse_area_);
  hover_.setTargetComponent(&browse_area_);

  scroll_bar_ = std::make_unique<OpenGlScrollBar>();
  addAndMakeVisible(scroll_bar_.get());
  addOpenGlComponent(scroll_bar_->getGlComponent());
  scroll_bar_->addListener(this);



  highlight_.setAdditive(true);
  hover_.setAdditive(true);


}

void PresetList::paintBackground(juce::Graphics& g) {
  int title_width = getTitleWidth();
  g.setColour(findColour(Skin::kWidgetBackground, true));
  g.fillRoundedRectangle(getLocalBounds().toFloat(), findValue(Skin::kBodyRounding));

  //int star_width = kStarWidthPercent * getWidth();
  int name_width = kNameWidthPercent * getWidth();
  int style_width = kStyleWidthPercent * getWidth();
  int author_width = kAuthorWidthPercent * getWidth();
  int date_width = kDateWidthPercent * getWidth();
  int row_height = getRowHeight();
  int text_padding = row_height / 2;

  g.saveState();
  g.setColour(findColour(Skin::kBody, true));
  g.reduceClipRegion(getLocalBounds().removeFromTop(title_width));
  juce::Rectangle<float> top = getLocalBounds().toFloat().removeFromTop(title_width * 2.0f);
  g.fillRoundedRectangle(top, findValue(Skin::kBodyRounding));
  g.restoreState();

  juce::Colour lighten = findColour(Skin::kLightenScreen, true);
  scroll_bar_->setColor(lighten);
  g.setColour(lighten);
  g.fillRect(0, 0, 1, title_width);
  g.fillRect(name_width, 0, 1, title_width);


    g.setColour(findColour(Skin::kTextComponentText, true));
    g.setFont(Fonts::instance()->proportional_regular().withPointHeight(title_width * 0.5f));
//
//  juce::Path star = Paths::star();
//  float star_draw_width = title_width * 0.8f;
//  float star_y = (title_width - star_draw_width) / 2.0f;
//  juce::Rectangle<float> star_bounds((star_width - star_draw_width) / 2.0f, star_y, star_draw_width, star_draw_width);
//  g.fillPath(star, star.getTransformToScaleToFit(star_bounds, true));

  g.drawText("Name", text_padding, 0, name_width, title_width, juce::Justification::centredLeft);
 // int style_x = star_width + name_width + text_padding;
 // g.drawText("Style", style_x, 0, style_width, title_width, juce::Justification::centredLeft);
 // int author_x = star_width + name_width + text_padding + style_width;
 // g.drawText("Author", author_x, 0, author_width, title_width, juce::Justification::centredLeft);
 // g.drawText("Date", getWidth() - date_width, 0, date_width - text_padding, title_width, juce::Justification::centredRight);

  paintBorder(g);
  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
}

void PresetList::resized() {
  static constexpr float kScrollBarWidth = 15.0f;

  int scroll_bar_width = kScrollBarWidth * getSizeRatio();
  int title_width = getTitleWidth();
  int scroll_bar_height = getHeight() - title_width;
  scroll_bar_->setBounds(getWidth() - scroll_bar_width, title_width, scroll_bar_width, scroll_bar_height);
  setScrollBarRange();

  browse_area_.setBounds(0, title_width, getWidth(), getHeight() - title_width);
}

void PresetList::sort() {
//  if (sort_column_ == kStar && sort_ascending_)
//    sortFileArray<FavoriteAscendingComparator>(presets_);
//  else if (sort_column_ == kStar && !sort_ascending_)
//    sortFileArray<FavoriteDescendingComparator>(presets_);
if (sort_column_ == kName && sort_ascending_)
    sortFileArray<FileNameAscendingComparator>(presets_);
  else if (sort_column_ == kName && !sort_ascending_)
    sortFileArray<FileNameDescendingComparator>(presets_);
//  else if (sort_column_ == kAuthor && sort_ascending_)
//    sortFileArrayWithCache<AuthorAscendingComparator>(presets_, &preset_info_cache_);
//  else if (sort_column_ == kAuthor && !sort_ascending_)
//    sortFileArrayWithCache<AuthorDescendingComparator>(presets_, &preset_info_cache_);
//  else if (sort_column_ == kStyle && sort_ascending_)
//    sortFileArrayWithCache<StyleAscendingComparator>(presets_, &preset_info_cache_);
//  else if (sort_column_ == kStyle && !sort_ascending_)
//    sortFileArrayWithCache<StyleDescendingComparator>(presets_, &preset_info_cache_);
//  else if (sort_column_ == kDate && sort_ascending_)
//    sortFileArray<FileDateAscendingComparator>(presets_);
//  else if (sort_column_ == kDate && !sort_ascending_)
//    sortFileArray<FileDateDescendingComparator>(presets_);
//
//  filter(filter_string_, filter_styles_);
}

void PresetList::setPresets(juce::Array<juce::File> presets) {
  presets_ = presets;
  sort();
  redoCache();
}

void PresetList::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
  view_position_ -= wheel.deltaY * kScrollSensitivity;
  view_position_ = std::max(0.0f, view_position_);
  int title_width = getTitleWidth();
  float scaled_height = getHeight() - title_width;
  int scrollable_range = getScrollableRange();
  view_position_ = std::min(view_position_, 1.0f * scrollable_range - scaled_height);
  viewPositionChanged();
  setScrollBarRange();
  finishRename();
}

int PresetList::getRowFromPosition(float mouse_position) {
  int title_width = getTitleWidth();
  
  return floorf((mouse_position + getViewPosition() - title_width) / getRowHeight());
}

void PresetList::mouseMove(const juce::MouseEvent& e) {
  hover_preset_ = getRowFromPosition(e.position.y);
  if (hover_preset_ >= filtered_presets_.size())
    hover_preset_ = -1;
}

void PresetList::mouseExit(const juce::MouseEvent& e) {
  hover_preset_ = -1;
}

void PresetList::respondToMenuCallback(int result) {
  if (click_preset_ < 0 || click_preset_ >= filtered_presets_.size())
    return;

  juce::File preset = filtered_presets_[click_preset_];
  if (result == kOpenFileLocation)
    preset.revealToUser();
  else if (result == kRename && rename_editor_) {
    renaming_preset_ = preset;
    int y = getTitleWidth() + click_preset_ * getRowHeight() - getViewPosition();
    rename_editor_->setBounds(0, y, kNameWidthPercent * getWidth(), getRowHeight());
    rename_editor_->setColour(juce::CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
    rename_editor_->setColour(juce::TextEditor::textColourId, findColour(Skin::kBodyText, true));
    rename_editor_->setColour(juce::TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
    rename_editor_->setColour(juce::TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
    rename_editor_->setText(renaming_preset_.getFileNameWithoutExtension());
    rename_editor_->setVisible(true);
    rename_editor_->grabKeyboardFocus();
    rename_editor_->selectAll();
  }
  else if (result == kDelete) {
    for (Listener* listener : listeners_)
      listener->deleteRequested(preset);
  }
}

void PresetList::menuClick(const juce::MouseEvent& e) {
  float click_y_position = e.position.y;
  int row = getRowFromPosition(click_y_position);

  if (row >= 0 && hover_preset_ >= 0) {
    click_preset_ = hover_preset_;
    PopupItems options;
    options.addItem(kOpenFileLocation, "Open juce::File Location");

    juce::File preset = filtered_presets_[click_preset_];
    if (preset.exists() && preset.hasWriteAccess()) {
      options.addItem(kRename, "Rename");
      options.addItem(kDelete, "Delete");
    }

    showPopupSelector(this, e.getPosition(), options, [=](int selection,int) { respondToMenuCallback(selection); });
  }
}

void PresetList::leftClick(const juce::MouseEvent& e) {
  int title_width = getTitleWidth();
  float click_y_position = e.position.y;
  float click_x_position = e.position.x;
  int row = getRowFromPosition(click_y_position);
  // int star_right = kStarWidthPercent * getWidth();

  if (click_y_position <= title_width) {
    int name_right = kNameWidthPercent * getWidth();
    int style_right = name_right + kStyleWidthPercent * getWidth();
    int author_right = style_right + kAuthorWidthPercent * getWidth();
    Column clicked_column;

//    if (click_x_position < star_right)
//      clicked_column = kStar;
    if (click_x_position < name_right)
        clicked_column = kName;
//    else if (click_x_position < style_right)
//      clicked_column = kStyle;
//    else if (click_x_position < author_right)
//      clicked_column = kAuthor;
//    else
//      clicked_column = kDate;
//
    if (clicked_column == sort_column_)
      sort_ascending_ = !sort_ascending_;
    else
      sort_ascending_ = true;
    sort_column_ = clicked_column;
    sort();
    redoCache();
  }
  else if (row < filtered_presets_.size() && row >= 0) {
    juce::File preset = filtered_presets_[row];
//    if (click_x_position < star_right) {
//      std::string path = preset.getFullPathName().toStdString();
//      if (favorites_.count(path)) {
//        favorites_.erase(path);
//        LoadSave::removeFavorite(preset);
//      }
//      else {
//        favorites_.insert(path);
//        LoadSave::addFavorite(preset);
//      }
//
//    }

      selected_preset_ = preset;

      for (Listener* listener : listeners_)
        listener->newPresetSelected(preset);

  }
}

void PresetList::mouseDown(const juce::MouseEvent& e) {
  if (e.mods.isPopupMenu())
    menuClick(e);
  else
    leftClick(e);
}

void PresetList::textEditorReturnKeyPressed(juce::TextEditor& text_editor) {
  if (renaming_preset_.exists())
    finishRename();
}

void PresetList::textEditorFocusLost(juce::TextEditor& text_editor) {
  if (renaming_preset_.exists())
    finishRename();
}

void PresetList::textEditorEscapeKeyPressed(juce::TextEditor& editor) {
  rename_editor_->setVisible(false);
}

void PresetList::scrollBarMoved(juce::ScrollBar* scroll_bar, double range_start) {
  view_position_ = range_start;
  viewPositionChanged();
}

void PresetList::setScrollBarRange() {
  static constexpr float kScrollStepRatio = 0.05f;

  int title_width = getTitleWidth();
  float scaled_height = getHeight() - title_width;
  scroll_bar_->setRangeLimits(0.0f, getScrollableRange());
  scroll_bar_->setCurrentRange(getViewPosition(), scaled_height, juce::NotificationType::dontSendNotification);
  scroll_bar_->setSingleStepSize(scroll_bar_->getHeight() * kScrollStepRatio);
  scroll_bar_->cancelPendingUpdate();
}

void PresetList::finishRename() {
  juce::String text = rename_editor_->getText();
  rename_editor_->setVisible(false);
  if (text.trim().isEmpty() || !renaming_preset_.exists())
    return;

  juce::File parent = renaming_preset_.getParentDirectory();
  juce::File new_file = parent.getChildFile(text + renaming_preset_.getFileExtension());
  renaming_preset_.moveFileTo(new_file);
  renaming_preset_ = juce::File();

  reloadPresets();
}

void PresetList::reloadPresets() {
  presets_.clear();
  if (current_folder_.exists() && current_folder_.isDirectory())
    current_folder_.findChildFiles(presets_, juce::File::findFiles, true, "*.xml");
  //else
    //LoadSave::getAllPresets(presets_);
  sort();
  redoCache();
}

void PresetList::shiftSelectedPreset(int indices) {
  int num_presets = static_cast<int>(filtered_presets_.size());
  if (num_presets == 0)
    return;

  int new_index = (getSelectedIndex() + num_presets + indices) % num_presets;
  selected_preset_ = filtered_presets_[new_index];
  for (Listener* listener : listeners_)
    listener->newPresetSelected(selected_preset_);
}

void PresetList::redoCache() {
  if (getWidth() <= 0 || getHeight() <= 0)
    return;

  int max = static_cast<int>(filtered_presets_.size()) - kNumCachedRows;
  int position = std::max(0, std::min<int>(cache_position_, max));
  loadBrowserCache(position, position + kNumCachedRows);
}

void PresetList::filter(juce::String filter_string, const std::set<std::string>& styles) {
  filter_string_ = filter_string.toLowerCase();
  filter_styles_ = styles;
  juce::StringArray tokens;
  tokens.addTokens(filter_string_, " ", "");
  filtered_presets_.clear();

  for (const juce::File& preset : presets_) {
    bool match = true;
    std::string path = preset.getFullPathName().toStdString();
//    if (!styles.empty()) {
//      std::string style = preset_info_cache_.getStyle(preset);
//      if (styles.count(style) == 0)
//        match = false;
//    }
//    if (match && tokens.size()) {
//      juce::String name = preset.getFileNameWithoutExtension().toLowerCase();
//      juce::String author = juce::String(preset_info_cache_.getAuthor(preset)).toLowerCase();
//
//      for (const juce::String& token : tokens) {
//        if (!name.contains(token) && !author.contains(token))
//          match = false;
//      }
//    }
//    if (match)
//      filtered_presets_.push_back(preset);
  }
  num_view_presets_ = static_cast<int>(filtered_presets_.size());

  setScrollBarRange();
}

int PresetList::getSelectedIndex() {
  for (int i = 0; i < filtered_presets_.size(); ++i) {
    if (selected_preset_ == filtered_presets_[i])
      return i;
  }
  return -1;
}

int PresetList::getScrollableRange() {
  int row_height = getRowHeight();
  int title_width = getTitleWidth();
  int presets_height = row_height * static_cast<int>(filtered_presets_.size());
  return std::max(presets_height, getHeight() - title_width);
}

void PresetList::initOpenGlComponents(OpenGlWrapper& open_gl) {
  for (int i = 0; i < kNumCachedRows; ++i) {
    rows_[i].setScissor(true);
    rows_[i].init(open_gl);
    rows_[i].setColor(juce::Colours::white);
  }

  highlight_.init(open_gl);
  hover_.init(open_gl);
  SynthSection::initOpenGlComponents(open_gl);
}

void PresetList::viewPositionChanged() {
  int row_height = getRowHeight();

  int last_cache_position = cache_position_;
  cache_position_ = getViewPosition() / row_height;
  int max = static_cast<int>(filtered_presets_.size() - kNumCachedRows);
  cache_position_ = std::max(0, std::min<int>(cache_position_, max));

  if (std::abs(cache_position_ - last_cache_position) >= kNumCachedRows)
    redoCache();
  else if (last_cache_position < cache_position_)
    loadBrowserCache(last_cache_position + kNumCachedRows, cache_position_ + kNumCachedRows);
  else if (last_cache_position > cache_position_)
    loadBrowserCache(cache_position_, last_cache_position);
}

void PresetList::loadBrowserCache(int start_index, int end_index) {
  int mult = getPixelMultiple();
  int row_height = getRowHeight() * mult;
  int image_width = getWidth() * mult;

  int text_padding = row_height / 2.0f;
  int star_x = text_padding;
  //int star_width = kStarWidthPercent * image_width;
  int name_x = text_padding;
  int name_width = kNameWidthPercent * image_width;
  int style_x = name_x + name_width;
  int style_width = kStyleWidthPercent * image_width;
  int author_x = style_x + style_width;
  int author_width = kAuthorWidthPercent * image_width;
  int date_width = kDateWidthPercent * image_width;
  int date_x = image_width - date_width + text_padding;

  end_index = std::min(static_cast<int>(filtered_presets_.size()), end_index);
  juce::Font font = Fonts::instance()->proportional_light().withPointHeight(row_height * 0.5f);

//  juce::Path star = Paths::star();
//  float star_draw_width = row_height * 0.8f;
//  float star_y = (row_height - star_draw_width) / 2.0f;
//  juce::Rectangle<float> star_bounds((star_width - star_draw_width) / 2.0f, star_y, star_draw_width, star_draw_width);
//  star.applyTransform(star.getTransformToScaleToFit(star_bounds, true));
//  juce::PathStrokeType star_stroke(1.0f, juce::PathStrokeType::curved);

  juce::Colour text_color = findColour(Skin::kTextComponentText, true);
 ;

  for (int i = start_index; i < end_index; ++i) {
    juce::Image row_image(juce::Image::ARGB, image_width, row_height, true);
    juce::Graphics g(row_image);

    juce::File preset = filtered_presets_[i];
    juce::String name = preset.getFileNameWithoutExtension();




//    g.strokePath(star, star_stroke);

    g.setColour(text_color);
    g.setFont(font);
    g.drawText(name, name_x, 0, name_width   - 2 * text_padding, row_height, juce::Justification::centredLeft, true);


    rows_[i % kNumCachedRows].setOwnImage(row_image);
  }
}

void PresetList::moveQuadToRow(OpenGlQuad& quad, int row, float y_offset) {
  int row_height = getRowHeight();
  float view_height = getHeight() - (int)getTitleWidth();
  float open_gl_row_height = 2.0f * row_height / view_height;
  float offset = row * open_gl_row_height;

  float y = 1.0f + y_offset - offset;
  quad.setQuad(0, -1.0f, y - open_gl_row_height, 2.0f, open_gl_row_height);
}

void PresetList::renderOpenGlComponents(OpenGlWrapper& open_gl, bool animate) {
  int title_width = getTitleWidth();
  float view_height = getHeight() - title_width;
  int row_height = getRowHeight();
  int num_presets = num_view_presets_;

  int view_position = getViewPosition();
  float y_offset = 2.0f * view_position / view_height;

  juce::Rectangle<int> view_bounds(0, title_width, getWidth(), getHeight() - title_width);
  OpenGlComponent::setViewPort(this, view_bounds, open_gl);

  float image_width = bitklavier::utils::nextPowerOfTwo(getWidth());
  float image_height = bitklavier::utils::nextPowerOfTwo(row_height);
  float width_ratio = image_width / getWidth();
  float height_ratio = image_height / row_height;

  float open_gl_row_height = height_ratio * 2.0f * row_height / view_height;
  int cache_position = std::max(0, std::min(cache_position_, num_presets - kNumCachedRows));
  for (int i = 0; i < kNumCachedRows && i < num_presets; ++i) {
    int row = cache_position + i;
    int cache_index = row % kNumCachedRows;
    float offset = (2.0f * row_height * row) / view_height;
    float y = 1.0f + y_offset - offset;

    juce::Rectangle<int> row_bounds(0, row_height * row - view_position + title_width, getWidth(), row_height);
    OpenGlComponent::setScissorBounds(this, row_bounds, open_gl);

    rows_[cache_index].setTopLeft(-1.0f, y);
    rows_[cache_index].setTopRight(-1.0f + 2.0f * width_ratio, y);
    rows_[cache_index].setBottomLeft(-1.0f, y - open_gl_row_height);
    rows_[cache_index].setBottomRight(-1.0f + 2.0f * width_ratio, y - open_gl_row_height);
    rows_[cache_index].drawImage(open_gl);
  }

  int selected_index = getSelectedIndex();
  if (selected_index >= 0) {
    moveQuadToRow(highlight_, selected_index, y_offset);
    highlight_.setColor(findColour(Skin::kWidgetPrimary1, true).darker(0.8f));
    highlight_.render(open_gl, animate);
  }

  if (hover_preset_ >= 0) {
    moveQuadToRow(hover_, hover_preset_, y_offset);
    hover_.setColor(findColour(Skin::kLightenScreen, true));
    hover_.render(open_gl, animate);
  }

  SynthSection::renderOpenGlComponents(open_gl, animate);
}

void PresetList::destroyOpenGlComponents(OpenGlWrapper& open_gl) {
  for (int i = 0; i < kNumCachedRows; ++i)
    rows_[i].destroy(open_gl);

  highlight_.destroy(open_gl);
  hover_.destroy(open_gl);
  SynthSection::destroyOpenGlComponents(open_gl);
}

PresetBrowser::PresetBrowser() : SynthSection("preset_browser") {
//  save_section_ = nullptr;
//  delete_section_ = nullptr;

  addKeyListener(this);

  preset_list_ = std::make_unique<PresetList>();
  preset_list_->addListener(this);
  addSubSection(preset_list_.get());

//  folder_list_ = std::make_unique<SelectionList>();
//  folder_list_->addFavoritesOption();
//  folder_list_->addListener(this);
//  addSubSection(folder_list_.get());
//  folder_list_->setPassthroughFolderName(LoadSave::kPresetFolderName);
//  std::vector<juce::File> directories = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);//LoadSave::getPresetDirectories();
//  juce::Array<juce::File> selections;
//  for (const juce::File& directory : directories)
//    selections.add(directory);
//  folder_list_->setSelections(selections);

//  for (int i = 0; i < LoadSave::kNumPresetStyles; ++i) {
//    style_buttons_[i] = std::make_unique<OpenGlToggleButton>(strings::kPresetStyleNames[i]);
//    style_buttons_[i]->addListener(this);
//    style_buttons_[i]->setLookAndFeel(TextLookAndFeel::instance());
//    addAndMakeVisible(style_buttons_[i].get());
//    addOpenGlComponent(style_buttons_[i]->getGlComponent());
//  }

//  store_button_ = std::make_unique<OpenGlToggleButton>("Store");
//  addButton(store_button_.get());
//  store_button_->setUiButton(true);
//  store_button_->setVisible(false);

  preset_text_ = std::make_shared<PlainTextComponent>("Preset", "Preset name");
  addOpenGlComponent(preset_text_);
  preset_text_->setFontType(PlainTextComponent::kLight);
  preset_text_->setJustification(juce::Justification::centredLeft);

//  author_text_ = std::make_unique<PlainTextComponent>("Author", "Author");
//  addOpenGlComponent(author_text_.get());
//  author_text_->setFontType(PlainTextComponent::kLight);
//  author_text_->setJustification(juce::Justification::centredLeft);
//
//#if !defined(NO_TEXT_ENTRY)
//  search_box_ = std::make_unique<OpenGlTextEditor>("Search");
//  search_box_->addListener(this);
//  search_box_->setSelectAllWhenFocused(true);
//  search_box_->setMultiLine(false, false);
//  search_box_->setJustification(juce::Justification::centredLeft);
//  addAndMakeVisible(search_box_.get());
//  addOpenGlComponent(search_box_->getImageComponent());
//
//  comments_ = std::make_unique<OpenGlTextEditor>("Comments");
//  comments_->setSelectAllWhenFocused(false);
//  comments_->setJustification(juce::Justification::topLeft);
//  comments_->setReadOnly(true);
//  addAndMakeVisible(comments_.get());
//  addOpenGlComponent(comments_->getImageComponent());
//  comments_->setMultiLine(true, true);
//#endif

  juce::Array<juce::File> presets;
//  LoadSave::getAllPresets(presets);
  preset_list_->setPresets(presets);

  setWantsKeyboardFocus(true);
  setMouseClickGrabsKeyboardFocus(true);
  setSkinOverride(Skin::kPresetBrowser);
}

PresetBrowser::~PresetBrowser() { }

void PresetBrowser::paintBackground(juce::Graphics& g) {
  juce::Rectangle<int> search_rect = getSearchRect();
  juce::Rectangle<int> info_rect = getInfoRect();
  paintBody(g, search_rect);
  paintBorder(g, search_rect);
  paintBody(g, info_rect);
  paintBorder(g, info_rect);

  int left_padding = kLeftPadding * size_ratio_;
  int top_padding = kTopPadding * size_ratio_;
  int middle_padding = kMiddlePadding * size_ratio_;

  int text_x = info_rect.getX() + left_padding;
  int text_width = info_rect.getWidth() - 2 * left_padding;
  int name_y = info_rect.getY() + top_padding;
  int name_height = kNameFontHeight * size_ratio_;
  int author_y = name_y + name_height + middle_padding;
  int author_height = kAuthorFontHeight * size_ratio_;
  int comments_y = author_y + author_height + 2 * middle_padding;

  g.setColour(findColour(Skin::kLightenScreen, true));
  g.drawRect(text_x, author_y, text_width, 1);
  g.drawRect(text_x, comments_y, text_width, 1);

  g.setColour(findColour(Skin::kWidgetBackground, true));
  int rounding = findValue(Skin::kWidgetRoundedCorner);
//  juce::Rectangle<float> folder_bounds = folder_list_->getBounds().toFloat().expanded(1);
//  g.fillRoundedRectangle(folder_bounds, rounding);

  paintChildrenBackgrounds(g);
}

void PresetBrowser::paintBackgroundShadow(juce::Graphics& g) {
  paintTabShadow(g, getSearchRect());
  paintTabShadow(g, getInfoRect());
}

void PresetBrowser::resized() {
  static constexpr float kBrowseWidthRatio = 0.68f;
  static constexpr float kSearchBoxRowHeightRatio = 1.3f;

  SynthSection::resized();

  juce::Colour empty_color = findColour(Skin::kBodyText, true);
  empty_color = empty_color.withAlpha(0.5f * empty_color.getFloatAlpha());

//  if (search_box_) {
//    search_box_->setTextToShowWhenEmpty(TRANS("Search"), empty_color);
//    search_box_->setColour(juce::CaretComponent::caretColourId, findColour(Skin::kTextEditorCaret, true));
//    search_box_->setColour(juce::TextEditor::textColourId, findColour(Skin::kBodyText, true));
//    search_box_->setColour(juce::TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
//    search_box_->setColour(juce::TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
//  }
//  if (comments_) {
//    comments_->setColour(juce::TextEditor::textColourId, findColour(Skin::kBodyText, true));
//    comments_->setColour(juce::TextEditor::highlightedTextColourId, findColour(Skin::kBodyText, true));
//    comments_->setColour(juce::TextEditor::highlightColourId, findColour(Skin::kTextEditorSelection, true));
//  }

  int padding = findValue(Skin::kLargePadding);
  int preset_list_width = getWidth() * kBrowseWidthRatio;
  preset_list_->setBounds(getWidth() - preset_list_width - padding, 0, preset_list_width, getHeight());
  if (isVisible())
    preset_list_->redoCache();

  juce::Rectangle<int> search_rect = getSearchRect();
  juce::Rectangle<int> info_rect = getInfoRect();
  int top_padding = kTopPadding * size_ratio_;
  int left_padding = kLeftPadding * size_ratio_;
  int middle_padding = kMiddlePadding * size_ratio_;

  int name_y = info_rect.getY() + top_padding;
  int name_height = kNameFontHeight * size_ratio_;
  int author_y = name_y + name_height + middle_padding;
  int author_height = kAuthorFontHeight * size_ratio_;
  int text_x = info_rect.getX() + left_padding;
  int text_width = info_rect.getWidth() - 2 * left_padding;
  preset_text_->setTextSize(name_height);
  preset_text_->setBounds(text_x, name_y - middle_padding, text_width, name_height + 2 * middle_padding);


  int style_filter_y = search_rect.getY() + top_padding;


  int widget_margin = getWidgetMargin();
  int style_button_height = preset_list_->getRowHeight();
  int style_filter_x = search_rect.getX() + left_padding;
  int style_filter_width = search_rect.getWidth() - 2 * left_padding + widget_margin;



  int folder_y = style_filter_y + 3 * style_button_height + 2 * widget_margin + top_padding + 1;

  setCommentsBounds();
}

void PresetBrowser::setCommentsBounds() {
  juce::Rectangle<int> info_rect = getInfoRect();
  int left_padding = kLeftPadding * size_ratio_;
  int top_padding = kTopPadding * size_ratio_;
  int top_info_height = (kNameFontHeight + kAuthorFontHeight + kMiddlePadding * 4) * size_ratio_;
  int width = info_rect.getWidth() - 2 * left_padding;

  int comments_x = info_rect.getX() + left_padding;
  int comments_y = info_rect.getY() + top_info_height + top_padding;
  int comments_height = info_rect.getBottom() - comments_y - top_padding;

  if (comments_)
    comments_->setBounds(comments_x, comments_y, width, comments_height);
}

void PresetBrowser::visibilityChanged() {
  SynthSection::visibilityChanged();


  if (isVisible())
  {
    preset_list_->redoCache();

    more_author_presets_.clear();
  }
  loadPresetInfo();
}

juce::Rectangle<int> PresetBrowser::getSearchRect() {
  juce::Rectangle<int> info_rect = getInfoRect();
  int padding = findValue(Skin::kLargePadding);
  int y = info_rect.getBottom() + padding;
  return juce::Rectangle<int>(0, y, info_rect.getWidth(), getHeight() - y);
}

juce::Rectangle<int> PresetBrowser::getInfoRect() {
  static constexpr float kInfoHeightRatio = 0.43f;
  int width = preset_list_->getX() - findValue(Skin::kLargePadding);
  int height = getHeight() * kInfoHeightRatio;
  return juce::Rectangle<int>(0, 0, width, height);
}

void PresetBrowser::loadPresets() {

  preset_list_->reloadPresets();
  preset_list_->filter("", std::set<std::string>());
}

void PresetBrowser::filterPresets() {
//  std::set<std::string> styles;
//
//
//  preset_list_->filter(search_box_->getText(), styles);
//  preset_list_->redoCache();
}

void PresetBrowser::textEditorTextChanged(juce::TextEditor& editor) {
  filterPresets();
}

void PresetBrowser::textEditorEscapeKeyPressed(juce::TextEditor& editor) {
  editor.setText("");
}



void PresetBrowser::buttonClicked(juce::Button* clicked_button) {
//  if (clicked_button == store_button_.get()) {
//    juce::String encoded_author = juce::URL::addEscapeChars(author_text_->getText().toStdString(), true);
//    encoded_author = encoded_author.replace("+", "%2B");
//
//    juce::URL url(juce::String(kPresetStoreUrl) + encoded_author);
//    url.launchInDefaultBrowser();
//  }
////  else
////    filterPresets();
}

bool PresetBrowser::keyPressed(const juce::KeyPress &key, juce::Component *origin) {
//  if (!isVisible())
//    return search_box_->hasKeyboardFocus(true);

  if (key.getKeyCode() == juce::KeyPress::escapeKey) {
    for (Listener* listener : listeners_)
      listener->hidePresetBrowser();
    return true;
  }
  if (key.getKeyCode() == juce::KeyPress::upKey || key.getKeyCode() == juce::KeyPress::leftKey) {
    loadPrevPreset();
    return true;
  }
  if (key.getKeyCode() == juce::KeyPress::downKey || key.getKeyCode() == juce::KeyPress::rightKey) {
    loadNextPreset();
    return true;
  }
//  return search_box_->hasKeyboardFocus(true);
}

bool PresetBrowser::keyStateChanged(bool is_key_down, juce::Component *origin) {
//  if (is_key_down)
//    return search_box_->hasKeyboardFocus(true);
  return false;
}

void PresetBrowser::jumpToPreset(int indices) {
  static const LoadSave::FileSorterAscending kFileSorter;

  juce::File parent = external_preset_.getParentDirectory();
  if (parent.exists()) {
    juce::Array<juce::File> presets;
    parent.findChildFiles(presets, juce::File::findFiles, false, juce::String("*.") + bitklavier::kPresetExtension);
    presets.sort(kFileSorter);
    int index = presets.indexOf(external_preset_);
    index = (index + indices + presets.size()) % presets.size();

    juce::File new_preset = presets[index];
    loadFromFile(new_preset);
    externalPresetLoaded(new_preset);
  }
  else
    preset_list_->shiftSelectedPreset(indices);
}

void PresetBrowser::loadPrevPreset() {
  jumpToPreset(-1);
}

void PresetBrowser::loadNextPreset() {
  jumpToPreset(1);
}

void PresetBrowser::externalPresetLoaded(juce::File file) {
  external_preset_ = file;
  setPresetInfo(file);
}

bool PresetBrowser::loadFromFile(juce::File& preset) {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return false;

  std::string error;
  if (parent->loadFromFile(preset, error)) {
    setPresetInfo(preset);

    return true;
  }
  return false;
}

void PresetBrowser::loadPresetInfo() {
  SynthGuiInterface* parent = findParentComponentOfClass<SynthGuiInterface>();
  if (parent == nullptr)
    return;

  juce::Colour background = findColour(Skin::kBody, true);
  juce::Colour lighten = findColour(Skin::kLightenScreen, true);
  lighten = background.overlaidWith(lighten);
  juce::Colour regular_text = findColour(Skin::kBodyText, true);

}

void PresetBrowser::setPresetInfo(juce::File& preset) {
  if (preset.exists())
  {
  }
}

void PresetBrowser::addListener(Listener* listener) {
  listeners_.push_back(listener);
}



//void PresetBrowser::newSelection(juce::File selection) {
//  if (selection.exists() && selection.isDirectory())
//    preset_list_->setCurrentFolder(selection);
//}
//

