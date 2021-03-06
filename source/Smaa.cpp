/**
 * Copyright (C) 2019, Jeremy Retailleau
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <string>
#include <sstream>
#include <vector>

#include "DDImage/PlanarIop.h"
#include "DDImage/Knobs.h"
#include "DDImage/Channel.h"
#include "DDImage/Blink.h"

#include "Blink/Blink.h"

#include "Smaa.h"
#include "AreaTex.h"
#include "SearchTex.h"

#include "SMAALumaEdges.h"
#include "SMAABlend.h"
#include "SMAANeighborhood.h"

static const char* const CLASS = "Smaa";
static const char* const HELP = "Subpixel Morphological Anti-Aliasing";

static DD::Image::Iop* build(Node *node) {
    return new Nuke::Smaa(node);
}
const DD::Image::Iop::Description Nuke::Smaa::description(
    CLASS, "Filter/Smaa", build
);


namespace Nuke {

const char* Smaa::Class() const { return CLASS; }
const char* Smaa::node_help() const { return HELP; }

Smaa::Smaa(Node* node)
    : DD::Image::PlanarIop(node)
    , _gpu_device(Blink::ComputeDevice::CurrentGPUDevice())
    , _use_gpu_if_available(true)
    , _edges_program(SMAALumaEdges)
    , _blend_program(SMAABlend)
    , _neighborhood_program(SMAANeighborhood)
{
    convert_texture(
        searchTexBytes, _search_texture,
        SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, 1
    );
    convert_texture(
        areaTexBytes, _area_texture,
        AREATEX_WIDTH, AREATEX_HEIGHT, 2
    );
}

void Smaa::knobs(DD::Image::Knob_Closure &f)
{
    Divider(f);
    Newline(f, "Local GPU: ");
    const bool hasGPU = _gpu_device.available();
    std::string gpu_name = hasGPU ? _gpu_device.name() : "Not available";
    Named_Text_knob(f, "gpu_name", gpu_name.c_str());
    Newline(f);
    Bool_knob(f, &_use_gpu_if_available, "use_gpu", "Use GPU if available");
    Divider(f);
}

void Smaa::_validate(bool)
{
    // Copy bbox channels etc from input0, which will validate it.
    copy_info();

    // Turn alpha channel on.
    set_out_channels(DD::Image::Mask_RGBA);
    info_.turn_on(DD::Image::Mask_RGBA);
}

void Smaa::getRequests(
    const DD::Image::Box &box,
    const DD::Image::ChannelSet &channels,
    int count,
    DD::Image::RequestOutput &data
) const
{
    data.request(&input0(), box, channels, count);
}

void Smaa::renderStripe(DD::Image::ImagePlane &output_plane)
{
    DD::Image::Box input_box = output_plane.bounds();
    input_box.intersect(input0().info());

    // Create image plane from input.
    DD::Image::ImagePlane input_plane(
        input_box,
        output_plane.packed(),
        output_plane.channels(),
        output_plane.nComps()
    );

    input0().fetchPlane(input_plane);
    output_plane.makeWritable();

    // Wrap planes as Blink images.
    Blink::Image output_image;
    Blink::Image input_image;
    bool success = (
        DD::Image::Blink::ImagePlaneAsBlinkImage(output_plane, output_image) &&
        DD::Image::Blink::ImagePlaneAsBlinkImage(input_plane, input_image)
    );

    // Check the fetch succeeded.
    if (!success) {
        error("Unable to fetch Blink image for image plane.");
        return;
    }

    bool using_gpu = _use_gpu_if_available && _gpu_device.available();

    // Get a reference to the ComputeDevice to do our processing on.
    Blink::ComputeDevice compute_device = using_gpu ?
        _gpu_device : Blink::ComputeDevice::CurrentCPUDevice();

    // Distribute input image from the device used by Nuke to compute device.
    Blink::Image input = input_image.distributeTo(compute_device);

    // Bind compute device to the calling thread.
    Blink::ComputeDeviceBinder binder(compute_device);

    // Make output images if GPU is being used, otherwise just use Nuke's planes.
    Blink::Image edges_tex = using_gpu ?
        output_image.makeLike(_gpu_device) : output_image;
    Blink::Image blend_tex = using_gpu ?
        output_image.makeLike(_gpu_device) : output_image;
    Blink::Image output = using_gpu ?
        output_image.makeLike(_gpu_device) : output_image;

    // Apply SMAA scripts.
    run_edges_detection(compute_device, input, edges_tex);
    run_blending_weight_calculation(compute_device, edges_tex, blend_tex);
    run_neighborhood_blending(compute_device, input, blend_tex, output);

    // Copy the result back to NUKE's output plane if GPU were used.
    if (using_gpu) {
        output_image.copyFrom(output);
    }
}

void Smaa::run_edges_detection(
    Blink::ComputeDevice device,
    const Blink::Image& input,
    const Blink::Image& edges_tex
)
{
    std::vector<Blink::Image> images;
    images.push_back(input);
    images.push_back(edges_tex);

    try {
        Blink::Kernel edges_kernel(
            _edges_program, device, images, kBlinkCodegenDefault
        );
        edges_kernel.iterate();
    }
    catch (Blink::ParseException& e) {
        std::ostringstream line_number;
        line_number << e.lineNumber();
        std::string message = (
            "Edge Detection (L" + line_number.str() + "): "
            + e.parseError()
        );
        error(message.c_str());
    }
    catch (Blink::Exception& e) {
        std::string message = "Edge Detection: " + e.userMessage();
        error(message.c_str());
    }
}

void Smaa::run_blending_weight_calculation(
    Blink::ComputeDevice device,
    const Blink::Image& edges_tex,
    const Blink::Image& blend_tex
)
{
    Blink::Image search_tex = create_search_texture(device);
    Blink::Image area_tex = create_area_texture(device);

    std::vector<Blink::Image> images;
    images.push_back(edges_tex);
    images.push_back(area_tex);
    images.push_back(search_tex);
    images.push_back(blend_tex);

    try {
        Blink::Kernel blend_kernel(
            _blend_program, device, images, kBlinkCodegenDefault
        );
        blend_kernel.iterate();
    }
    catch (Blink::ParseException& e) {
        std::ostringstream line_number;
        line_number << e.lineNumber();
        std::string message = (
            "Blend Computation (L" + line_number.str() + "): "
            + e.parseError()
        );
        error(message.c_str());
    }
    catch (Blink::Exception& e) {
        std::string message = "Blend Computation: " + e.userMessage();
        error(message.c_str());
    }
}

void Smaa::run_neighborhood_blending(
    Blink::ComputeDevice device,
    const Blink::Image& input,
    const Blink::Image& blend_tex,
    const Blink::Image& output
)
{
    std::vector<Blink::Image> images;
    images.push_back(input);
    images.push_back(blend_tex);
    images.push_back(output);

    try {
        Blink::Kernel neighborhood_kernel(
            _neighborhood_program, device, images, kBlinkCodegenDefault
        );
        neighborhood_kernel.iterate();
    }
    catch (Blink::ParseException& e) {
        std::ostringstream line_number;
        line_number << e.lineNumber();
        std::string message = (
            "Neighborhood Blending (L" + line_number.str() + "): "
            + e.parseError()
        );
        error(message.c_str());
    }
    catch (Blink::Exception& e) {
        std::string message = "Neighborhood Blending: " + e.userMessage();
        error(message.c_str());
    }
}

Blink::Image Smaa::create_search_texture(Blink::ComputeDevice device) {
    Blink::Rect rect(0, 0, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT);
    Blink::PixelInfo pixelInfo(1, kBlinkDataFloat);
    Blink::ImageInfo imageInfo(rect, pixelInfo);

    Blink::Image image = Blink::Image(imageInfo, device);
    Blink::BufferDesc bufferDesc(
        sizeof(float),
        sizeof(float) * SEARCHTEX_WIDTH,
        sizeof(float)
    );
    image.copyFromBuffer(_search_texture.data(), bufferDesc);
    return image;
}

Blink::Image Smaa::create_area_texture(Blink::ComputeDevice device) {
    Blink::Rect rect(0, 0, AREATEX_WIDTH, AREATEX_HEIGHT);
    Blink::PixelInfo pixelInfo(2, kBlinkDataFloat);
    Blink::ImageInfo imageInfo(rect, pixelInfo);

    Blink::Image image = Blink::Image(imageInfo, device);
    Blink::BufferDesc bufferDesc(
        sizeof(float) * 2,
        sizeof(float) * AREATEX_WIDTH,
        sizeof(float)
    );
    image.copyFromBuffer(_area_texture.data(), bufferDesc);
    return image;
}

void Smaa::convert_texture(
    const unsigned char* source, std::vector<float>& destination,
    int width, int height, int channelNumber
) {
    const int size = width * height * channelNumber;
    destination.reserve(size);

    for (int index = 0; index < size; index++) {
        destination.push_back((float) source[index]);
    }
}

} // namespace Nuke
