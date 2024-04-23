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

#include "OpenGLImageFileComponent.h"
#include "../synthesis/framework/utils.h"
#include "BinaryData.h"
OpenGlImageFileComponent::OpenGlImageFileComponent(String name) : OpenGlComponent(name), component_(nullptr),
                                                          active_(true), static_image_(false),
                                                          paint_entire_component_(true) {
   image_.setTopLeft(-1.0f, 1.0f);
   image_.setTopRight(1.0f, 1.0f);
   image_.setBottomLeft(-1.0f, -1.0f);
   image_.setBottomRight(1.0f, -1.0f);
   image_.setColor(Colours::white);

   if (name == "")
       setInterceptsMouseClicks(false, false);
}

void OpenGlImageFileComponent::redrawImage(bool force) {
   if (!active_)
       return;

   Component* component = component_ ? component_ : this;

   float pixel_scale = Desktop::getInstance().getDisplays().getDisplayForPoint(getScreenPosition())->scale;
   int width = component->getWidth() * pixel_scale;
   int height = component->getHeight() * pixel_scale;
   if (width <= 0 || height <= 0)
       return;

   //bool new_image = draw_image_ == nullptr || draw_image_->getWidth() != width || draw_image_->getHeight() != height;
   //if (!new_image && (static_image_ || !force))
   //    return;

   image_.lock();

  // if (new_image)
       //draw_image_ = std::make_unique<Image>(ImageCache::getFromMemory(BinaryData::direct_icon_png, BinaryData::direct_icon_pngSize));
       //draw_image_ = image_->;

   //draw_image_->clear(Rectangle<int>(0, 0, width, height));
//   Graphics g(image_.g);
//   g.addTransform(AffineTransform::scale(pixel_scale));
//   paintToImage(g);
   //image_.setImage(draw_image_.get());
    //image_.drawImage()
   float gl_width = bitklavier::utils::nextPowerOfTwo(width);
   float gl_height = bitklavier::utils::nextPowerOfTwo(height);
   float width_ratio = gl_width / width;
   float height_ratio = gl_height / height;

   float right = -1.0f + 2.0f * width_ratio;
   float bottom = 1.0f - 2.0f * height_ratio;
   image_.setTopRight(right, 1.0f);
   //image_.setTopLeft(-1.0, 1.0f * width_ratio);
   image_.setBottomLeft(-1.0f, bottom);
   image_.setBottomRight(right, bottom);
   image_.unlock();
}

void OpenGlImageFileComponent::init(OpenGlWrapper& open_gl) {
   image_.init(open_gl);
}

void OpenGlImageFileComponent::render(OpenGlWrapper& open_gl, bool animate) {
   Component* component = component_ ? component_ : this;
   if (!active_ || !setViewPort(component, open_gl) || !component->isVisible() ) //|| draw_image_ == nullptr)
       return;

   image_.drawImage(open_gl);
}

void OpenGlImageFileComponent::destroy(OpenGlWrapper& open_gl) {
   image_.destroy(open_gl);
}
