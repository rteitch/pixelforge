#include "Image.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <stdexcept>
#include <algorithm>

namespace PixelForge {

// ---- Constructors / Destructor ----

Image::Image() = default;

Image::Image(const cv::Mat& mat) : mat_(mat) {}

Image::Image(int width, int height, int channels)
    : mat_(cv::Mat::zeros(height, width, CV_MAKETYPE(CV_8U, channels))) {}

Image::Image(const Image& other) : mat_(other.mat_) {}

Image::Image(Image&& other) noexcept : mat_(std::move(other.mat_)) {}

Image& Image::operator=(const Image& other) {
    if (this != &other) {
        mat_ = other.mat_;
    }
    return *this;
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        mat_ = std::move(other.mat_);
    }
    return *this;
}

Image::~Image() = default;

// ---- Factory ----

Image Image::fromFile(const std::string& path) {
    cv::Mat img = cv::imread(path, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        throw std::runtime_error("Failed to load image: " + path);
    }
    // Convert BGR (OpenCV default) to RGB
    if (img.channels() == 3) {
        cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    } else if (img.channels() == 4) {
        cv::cvtColor(img, img, cv::COLOR_BGRA2RGBA);
    }
    return Image(img);
}

Image Image::fromBuffer(const uint8_t* data, size_t size) {
    cv::Mat rawData(1, static_cast<int>(size), CV_8UC1, const_cast<uint8_t*>(data));
    cv::Mat img = cv::imdecode(rawData, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        throw std::runtime_error("Failed to decode image from buffer");
    }
    if (img.channels() == 3) {
        cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    } else if (img.channels() == 4) {
        cv::cvtColor(img, img, cv::COLOR_BGRA2RGBA);
    }
    return Image(img);
}

Image Image::empty(int width, int height, int channels) {
    return Image(cv::Mat::zeros(height, width, CV_MAKETYPE(CV_8U, channels)));
}

// ---- Properties ----

int Image::width() const {
    return mat_.cols;
}

int Image::height() const {
    return mat_.rows;
}

int Image::channels() const {
    return mat_.channels();
}

Size2i Image::size() const {
    return {mat_.cols, mat_.rows};
}

bool Image::isEmpty() const {
    return mat_.empty();
}

bool Image::hasAlpha() const {
    return mat_.channels() == 4;
}

size_t Image::memoryBytes() const {
    return mat_.total() * mat_.elemSize();
}

// ---- Access ----

cv::Mat& Image::mat() {
    return mat_;
}

const cv::Mat& Image::mat() const {
    return mat_;
}

cv::Mat Image::cloneMat() const {
    return mat_.clone();
}

// ---- Color space ----

Image Image::toColorSpace(ColorSpace space) const {
    if (mat_.empty()) return *this;

    cv::Mat result;
    switch (space) {
        case ColorSpace::sRGB:
            return *this; // already sRGB

        case ColorSpace::LinearRGB: {
            // sRGB to linear: apply inverse gamma
            cv::Mat floatImg;
            mat_.convertTo(floatImg, CV_32FC3, 1.0 / 255.0);
            // Approximate sRGB to linear
            cv::pow(floatImg, 2.2, floatImg);
            floatImg.convertTo(result, CV_8UC3, 255.0);
            break;
        }

        case ColorSpace::LAB: {
            cv::cvtColor(mat_, result, cv::COLOR_RGB2Lab);
            break;
        }

        case ColorSpace::HSV: {
            cv::cvtColor(mat_, result, cv::COLOR_RGB2HSV);
            break;
        }

        case ColorSpace::YCrCb: {
            cv::cvtColor(mat_, result, cv::COLOR_RGB2YCrCb);
            break;
        }
    }
    return Image(result);
}

Image Image::toGrayscale() const {
    if (mat_.empty()) return *this;
    if (mat_.channels() == 1) return *this;

    cv::Mat gray;
    cv::cvtColor(mat_, gray, cv::COLOR_RGB2GRAY);
    cv::cvtColor(gray, gray, cv::COLOR_GRAY2RGB);
    return Image(gray);
}

Image Image::toBGRA() const {
    if (mat_.empty()) return *this;
    cv::Mat bgra;
    if (mat_.channels() == 3) {
        cv::cvtColor(mat_, bgra, cv::COLOR_RGB2BGRA);
    } else if (mat_.channels() == 4) {
        cv::cvtColor(mat_, bgra, cv::COLOR_RGBA2BGRA);
    } else {
        cv::cvtColor(mat_, bgra, cv::COLOR_GRAY2BGRA);
    }
    return Image(bgra);
}

// ---- Resize ----

Image Image::resized(int targetWidth, int targetHeight, int interpolation) const {
    if (mat_.empty()) return *this;
    cv::Mat result;
    cv::resize(mat_, result, cv::Size(targetWidth, targetHeight), 0, 0, interpolation);
    return Image(result);
}

Image Image::resizedToFit(int maxDim) const {
    if (mat_.empty()) return *this;
    int w = mat_.cols;
    int h = mat_.rows;
    if (w <= maxDim && h <= maxDim) return *this;

    float scale = static_cast<float>(maxDim) / static_cast<float>(std::max(w, h));
    int newW = static_cast<int>(w * scale);
    int newH = static_cast<int>(h * scale);
    return resized(newW, newH);
}

Image Image::downsampledForPreview(int maxPixels) const {
    if (mat_.empty()) return *this;
    int64_t pixels = static_cast<int64_t>(mat_.cols) * mat_.rows;
    if (pixels <= maxPixels) return *this;

    float scale = std::sqrt(static_cast<float>(maxPixels) / static_cast<float>(pixels));
    int newW = std::max(1, static_cast<int>(mat_.cols * scale));
    int newH = std::max(1, static_cast<int>(mat_.rows * scale));
    return resized(newW, newH);
}

// ---- Crop / Rotate ----

Image Image::cropped(const Rect2i& roi) const {
    if (mat_.empty()) return *this;
    // Clamp to image bounds
    int x = std::max(0, roi.x);
    int y = std::max(0, roi.y);
    int w = std::min(roi.width, mat_.cols - x);
    int h = std::min(roi.height, mat_.rows - y);
    if (w <= 0 || h <= 0) return Image();

    cv::Rect cvRoi(x, y, w, h);
    return Image(mat_(cvRoi).clone());
}

Image Image::rotated90(int times) const {
    if (mat_.empty()) return *this;
    times = ((times % 4) + 4) % 4; // normalize to 0-3
    cv::Mat result;
    switch (times) {
        case 0: return *this;
        case 1: cv::rotate(mat_, result, cv::ROTATE_90_CLOCKWISE); break;
        case 2: cv::rotate(mat_, result, cv::ROTATE_180); break;
        case 3: cv::rotate(mat_, result, cv::ROTATE_90_COUNTERCLOCKWISE); break;
    }
    return Image(result);
}

Image Image::rotated(float angleDegrees, Color3u8 fillColor) const {
    if (mat_.empty()) return *this;
    cv::Point2f center(mat_.cols / 2.0f, mat_.rows / 2.0f);
    cv::Mat rotMat = cv::getRotationMatrix2D(center, -angleDegrees, 1.0);

    cv::Mat result;
    cv::Scalar fill(fillColor.b, fillColor.g, fillColor.r); // BGR
    cv::warpAffine(mat_, result, rotMat, mat_.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, fill);
    return Image(result);
}

// ---- Pixel access ----

uint8_t* Image::row(int y) {
    return mat_.ptr<uint8_t>(y);
}

const uint8_t* Image::row(int y) const {
    return mat_.ptr<uint8_t>(y);
}

Color3u8 Image::pixelAt(int x, int y) const {
    if (mat_.empty() || x < 0 || y < 0 || x >= mat_.cols || y >= mat_.rows) {
        return {0, 0, 0};
    }
    const uint8_t* p = mat_.ptr<uint8_t>(y) + x * mat_.channels();
    if (mat_.channels() >= 3) {
        return {p[0], p[1], p[2]};
    } else {
        return {p[0], p[0], p[0]};
    }
}

void Image::setPixelAt(int x, int y, Color3u8 color) {
    if (mat_.empty() || x < 0 || y < 0 || x >= mat_.cols || y >= mat_.rows) return;
    uint8_t* p = mat_.ptr<uint8_t>(y) + x * mat_.channels();
    if (mat_.channels() >= 3) {
        p[0] = color.r;
        p[1] = color.g;
        p[2] = color.b;
        if (mat_.channels() == 4) p[3] = 255;
    } else {
        p[0] = static_cast<uint8_t>(0.299f * color.r + 0.587f * color.g + 0.114f * color.b);
    }
}

// ---- Deep copy ----

Image Image::deepCopy() const {
    return Image(mat_.clone());
}

// ---- Save ----

bool Image::save(const std::string& path, int jpegQuality) const {
    if (mat_.empty()) return false;

    // Convert back to BGR for OpenCV save
    cv::Mat saveMat;
    if (mat_.channels() == 3) {
        cv::cvtColor(mat_, saveMat, cv::COLOR_RGB2BGR);
    } else if (mat_.channels() == 4) {
        cv::cvtColor(mat_, saveMat, cv::COLOR_RGBA2BGRA);
    } else {
        saveMat = mat_;
    }

    std::vector<int> params;
    std::string ext = path.substr(path.rfind('.'));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".jpg" || ext == ".jpeg") {
        params = {cv::IMWRITE_JPEG_QUALITY, std::clamp(jpegQuality, 1, 100)};
    } else if (ext == ".png") {
        params = {cv::IMWRITE_PNG_COMPRESSION, 6};
    } else if (ext == ".webp") {
        params = {cv::IMWRITE_WEBP_QUALITY, std::clamp(jpegQuality, 1, 100)};
    }

    return cv::imwrite(path, saveMat, params);
}

} // namespace PixelForge