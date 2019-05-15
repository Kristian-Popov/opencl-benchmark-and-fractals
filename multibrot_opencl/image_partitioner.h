#pragma once

#include <algorithm>
#include <stdexcept>
#include <vector>

/*
Partitions an abstract two-dimension work assignment into batches of rectangular form,
suitable for execution.
The entire area is split into pieces called fragments (their size in pixels is configurable by
constructor parameters). Area width and height doesn't have to be perfectly divisible
by fragment width or height.
Code tries to use full fragments (edge fragments may be smaller than requested), starting with
left top ones, moving to the right and then down, row by row. Row height is always one fragment.
*/
class ImagePartitioner {
public:
    struct Segment {
        size_t x = 0;
        size_t y = 0;
        size_t width_pix = 0;
        size_t height_pix = 0;

        bool IsEmpty() const { return width_pix == 0 || height_pix == 0; }
    };

    ImagePartitioner(
        size_t area_width_pix, size_t area_height_pix, size_t fragment_width_pix,
        size_t fragment_height_pix)
        : area_width_pix_(area_width_pix),
          area_height_pix_(area_height_pix),
          fragment_width_pix_(fragment_width_pix),
          fragment_height_pix_(fragment_height_pix) {
        if (fragment_width_pix > area_width_pix || fragment_height_pix > area_height_pix) {
            throw std::invalid_argument("Reqested fragment that is larger than the whole area.");
        }
    }

    // Returns a segment by a given parameters.
    // preferred_fragment_count is a preferred number of fragments in a segment
    // Under no circumstances segment larger than preferred is returned.
    Segment Partition(size_t preferred_fragment_count) {
        Segment result;
        if (!segments_.empty()) {
            // First check is last segment fills the whole row
            const Segment& last_segment = segments_.back();
            result = last_segment;
            if (last_segment.x + last_segment.width_pix == area_width_pix_) {
                // Start from a new line
                result.x = 0;
                result.y = last_segment.y + last_segment.height_pix;
            } else {
                // Continue current line
                result.x += last_segment.width_pix;
            }
        }
        result.width_pix =
            std::min(preferred_fragment_count * fragment_width_pix_, area_width_pix_ - result.x);
        result.height_pix = std::min(fragment_height_pix_, area_height_pix_ - result.y);
        segments_.push_back(result);
        return result;
    }

    void Reset() { segments_.clear(); }

private:
    size_t area_width_pix_;
    size_t area_height_pix_;
    size_t fragment_width_pix_;
    size_t fragment_height_pix_;
    // Segments are stored in a row-by-row basis in ascending order.
    std::vector<Segment> segments_;
    // TODO since we're deterministic in selecting segments, we can map segment
    // coordinates to a number of a last segment and then store it
};
