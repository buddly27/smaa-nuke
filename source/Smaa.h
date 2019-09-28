/**
 * Copyright (C) 2019, Jeremy Retailleau
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef SMAA_NUKE_H
#define SMAA_NUKE_H

#include "DDImage/PlanarIop.h"
#include "DDImage/Knobs.h"
#include "DDImage/NukeWrapper.h"
#include "DDImage/Blink.h"

#include "Blink/Blink.h"


namespace Nuke {

class Smaa : public DD::Image::PlanarIop
{
public:
    Smaa(Node *node);
    virtual ~Smaa() {}

    const char *Class() const;
    const char *node_help() const;

    static const Iop::Description description;

    int maximum_inputs() const { return 1; }
    int minimum_inputs() const { return 1; }

protected:
    virtual void knobs(DD::Image::Knob_Callback f);
    void _validate(bool);

    void getRequests(
        const DD::Image::Box &box,
        const DD::Image::ChannelSet &channels,
        int count, DD::Image::RequestOutput &data
    ) const;

    void renderStripe(DD::Image::ImagePlane &output_plane);

    void run_edges_detection(
        Blink::ComputeDevice device,
        const Blink::Image& input,
        const Blink::Image& edges_tex
    );

    void run_blending_weight_calculation(
        Blink::ComputeDevice device,
        const Blink::Image& edges_tex,
        const Blink::Image& blend_tex
    );

    void run_neighborhood_blending(
        Blink::ComputeDevice device,
        const Blink::Image& input,
        const Blink::Image& blend_tex,
        const Blink::Image& output
    );

    Blink::Image create_search_texture(Blink::ComputeDevice device);
    Blink::Image create_area_texture(Blink::ComputeDevice device);

    // Convert texture from unsigned char to float array.
    static void convert_texture(
        const unsigned char* source, std::vector<float>& destination,
        int width, int height, int channelNumber
    );

private:
    Blink::ComputeDevice _gpu_device;
    bool _use_gpu_if_available;

    Blink::ProgramSource _edges_program;
    Blink::ProgramSource _blend_program;
    Blink::ProgramSource _neighborhood_program;

    std::vector<float> _search_texture;
    std::vector<float> _area_texture;
};

} // namespace Nuke

#endif