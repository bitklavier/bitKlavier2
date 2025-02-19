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

#include "border_bounds_constrainer.h"
#include "FullInterface.h"

void BorderBoundsConstrainer::checkBounds(juce::Rectangle<int>& bounds, const juce::Rectangle<int>& previous,
                                          const juce::Rectangle<int>& limits,
                                          bool stretching_top, bool stretching_left,
                                          bool stretching_bottom, bool stretching_right) {
  border_.subtractFrom(bounds);
  double aspect_ratio = getFixedAspectRatio();

  juce::ComponentBoundsConstrainer::checkBounds(bounds, previous, limits,
                                          stretching_top, stretching_left,
                                          stretching_bottom, stretching_right);

    juce::Rectangle<int> display_area = juce::Desktop::getInstance().getDisplays().getTotalBounds(true);
  if (gui_) {
    juce::ComponentPeer* peer = gui_->getPeer();
    if (peer)
      peer->getFrameSize().subtractFrom(display_area);
  }

  if (display_area.getWidth() < bounds.getWidth()) {
    int new_width = display_area.getWidth();
    int new_height = std::round(new_width / aspect_ratio);
    bounds.setWidth(new_width);
    bounds.setHeight(new_height);

  }
  if (display_area.getHeight() < bounds.getHeight()) {
    int new_height = display_area.getHeight();
    int new_width = std::round(new_height * aspect_ratio);
    bounds.setWidth(new_width);
    bounds.setHeight(new_height);

  }
  gui_->resize_image_ =  gui_->resize_image_.rescaled(bounds.getWidth(), bounds.getHeight(),juce::Graphics::ResamplingQuality::highResamplingQuality);
  border_.addTo(bounds);
}

void BorderBoundsConstrainer::resizeStart() {
  if (gui_)
  {
      const auto imageToUse = [&]() -> juce::Image
      {
          const auto scaleFactor = 2.0;
          auto image = gui_->createComponentSnapshot (gui_->getBounds(), false, 1)
                  .convertedToFormat (juce::Image::ARGB);

          return { image };
      }();
      if (!gui_->resize_image_.isValid())
          gui_->resize_image_ = imageToUse;
      gui_->open_gl_context_.detach();
      gui_->enableRedoBackground(false);
      gui_->resizing = true;
  }

}

void BorderBoundsConstrainer::resizeEnd() {
  if (gui_) {

    //LoadSave::saveWindowSize(gui_->getWidth() / (1.0f * vital::kDefaultWindowWidth));

      gui_->open_gl_context_.setContinuousRepainting(true);
      gui_->open_gl_context_.setOpenGLVersionRequired(juce::OpenGLContext::openGL3_2);
      gui_->open_gl_context_.setSwapInterval(0);
      gui_->open_gl_context_.setRenderer(gui_);
      //componentpaintingenabled fixes flickering
      gui_->open_gl_context_.setComponentPaintingEnabled(false); // set to true, and the non-OpenGL components will draw
      gui_->open_gl_context_.attachTo(*gui_);

      gui_->resize_image_ = juce::Image();
      gui_->resizing = false;
  }
}
