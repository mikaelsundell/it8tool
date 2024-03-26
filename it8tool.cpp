//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-present Mikael Sundell.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <cstdio>
#include <cmath>

// imath
#include <Imath/ImathMatrix.h>
#include <Imath/ImathVec.h>

// openimageio
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/argparse.h>
#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/sysutil.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

// eigen
#include <eigen3/Eigen/Dense>

// rawtoaces
#include <rawtoaces/acesrender.h>

using namespace OIIO;

// prints
template <typename T>
static void
print_info(std::string param, const T& value)
{
    std::cout << "info: " << param << value << std::endl;
}

template <typename T>
static void
print_warning(std::string param, const T& value)
{
    std::cout << "warning: " << param << value << std::endl;
}

template <typename T>
static void
print_error(std::string param, const T& value)
{
    std::cerr << "error: " << param << value << std::endl;
}

// it8 tool
struct IT8Tool
{
    bool help = false;
    bool verbose = false;
    bool debug;
    std::string inputfile;
    std::string outputfile;
    std::string referencefile;
    std::string outputdatafile;
    std::string outputrawpatchfile;
    std::string outputrawreferencefile;
    std::string outputcalibrationmatrixfile;
    std::string outputcalibrationlutfile;
    float xoffset = 0.0f;
    float yoffset = 0.0f;
    float xscale = 0.8f;
    float yscale = 0.8f;
    bool rotate90;
    bool rotate180;
    bool rotate270;
    int code = EXIT_SUCCESS;
};

static IT8Tool tool;

static int
set_inputfile(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.inputfile = argv[1];
    return 0;
}

static int
set_ireferencefile(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.referencefile = argv[1];
    return 0;
}

static int
set_offset(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    std::istringstream iss(argv[1]);
    iss >> tool.xoffset;
    iss.ignore(); // Ignore the comma
    iss >> tool.yoffset;
    if (iss.fail()) {
        print_error("could not parse it8 offset from string", argv[1]);
        return 1;
    } else {
        return 0;
    }
}

static int
set_scale(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    std::istringstream iss(argv[1]);
    iss >> tool.xscale;
    iss.ignore(); // Ignore the comma
    iss >> tool.yscale;
    if (iss.fail()) {
        print_error("could not parse it8 scale from string", argv[1]);
        return 1;
    } else {
        return 0;
    }
}

// --it8rotate90
static void
set_rotate90(cspan<const char*> argv)
{
    OIIO_DASSERT(argv.size() == 1);
    tool.rotate90 = true;
}

// --it8rotate180
static void
set_rotate180(cspan<const char*> argv)
{
    OIIO_DASSERT(argv.size() == 1);
    tool.rotate180 = true;
}

// --it8rotate270
static void
set_rotate270(cspan<const char*> argv)
{
    OIIO_DASSERT(argv.size() == 1);
    tool.rotate270 = true;
}

// --help
static void
print_help(ArgParse& ap)
{
    ap.print_help();
}

// utils - characters
int int_from_char(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 1;
    } else if (c >= 'a' && c <= 'z') {
        return c - 'a' + 1;
    }
    return -1;
}

char char_from_int(int i) {
    if (i >= 1 && i <= 26) {
        return 'A' + i - 1;
    } else if (i >= 27 && i <= 52) {
        return 'a' + i - 27;
    }
    return '\0';
}

// utils - strings
std::string pad_from_int(int p, int i) {
    std::ostringstream oss;
    oss << std::setw(p) << std::setfill('0') << i;
    return oss.str();
}

std::string percentage_from_float(float value, int precision = 2) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << (value * 100.0f) << "%";
    return oss.str();
}

float clamp_from_float(float value, float minVal, float maxVal) {
    return std::max(minVal, std::min(value, maxVal));
}

std::string nocr_from_str(const std::string& str) {
    size_t pos = str.find_last_not_of('\r');
    if (pos != std::string::npos) {
        return str.substr(0, pos + 1);
    }
    return str;
}

// utils - math
double radians_from_degrees(double degrees) {
    return degrees * M_PI / 180.0;
}

// utils - color spaces
Imath::Vec3<float> multiply_from_matrix(const Imath::Vec3<float>& src, const Imath::Matrix33<float>& matrix) {
    return src * matrix.transposed(); // imath row-order from column-order convention
}

Imath::Vec3<float> ciexyzd65_from_aces2065_1(const Imath::Vec3<float>& src) {
    // For ACES2065-1
    // ACES Primaries:
    //  Red(0.7347, 0.2653)
    //  Green(0.0000, 1.0000)
    //  Blue(0.0001, -0.0770)
    // ACES White Point
    //  White(0.32168, 0.33767)
    Imath::Matrix33<float> matrix(
      0.9525523959f, 0.0000000000f, 0.0000936786f,
      0.3439664498f, 0.7281660966f, -0.0721325464f,
      0.0000000000f, 0.0000000000f, 1.0088251844f
    );
    return multiply_from_matrix(src, matrix);
}

Imath::Vec3<float> aces2065_1_from_ciexyz_d65(const Imath::Vec3<float>& src) {
    Imath::Matrix33<float> matrix(
      1.0498110175f, 0.0000000000f, -0.0000974845f,
      -0.4959030231f, 1.3733130458f, 0.0982400361f,
      0.0000000000f, 0.0000000000, 0.9912520182
    );
    return multiply_from_matrix(src, matrix);
}

Imath::Vec3<float> ciexyzd65_from_D50(const Imath::Vec3<float>& src) {
    
    // http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
    Imath::Matrix33<float> matrix(
      0.9555766f, -0.0230393f, 0.0631636f,
      -0.0282895f, 1.0099416f, 0.0210077f,
      0.0122982f, -0.020483f, 1.3299098f);
    return multiply_from_matrix(src, matrix);
}

Imath::Vec3<float> ciexyzd50_from_D65(const Imath::Vec3<float>& src) {
    
    // http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
    Imath::Matrix33<float> matrix(
      1.0478112f, 0.0228866f, -0.0501270f,
      0.0295424f, 0.9904844f, -0.0170491f,
      -0.0092345f, 0.0150436f, 0.7521316f);
    return multiply_from_matrix(src, matrix);
}

Imath::Vec3<float> ciexyzd65_from_lab(const Imath::Vec3<float>& src) {
    // D65 reference white values
    const double Xn = 0.95047;
    const double Yn = 1.00000;
    const double Zn = 1.08883;

    double fy = (src.x + 16.0) / 116.0;
    double fx = fy + (src.y / 500.0);
    double fz = fy - (src.z / 200.0);

    double fx3 = std::pow(fx, 3.0);
    double fz3 = std::pow(fz, 3.0);

    float X = (fx3 > 0.008856) ? fx3 : ((fx - 16.0 / 116.0) / 7.787);
    float Y = (src.x > 0.008856) ? std::pow(((src.x + 16.0) / 116.0), 3.0) : (src.x / 903.3);
    float Z = (fz3 > 0.008856) ? fz3 : ((fz - 16.0 / 116.0) / 7.787);

    X *= Xn;
    Y *= Yn;
    Z *= Zn;
    
    return Imath::Vec3<float>(X, Y, Z);
}

// utils - region of interest
ROI offset(const ROI roi, int dx, int dy)
{
    return ROI(
        roi.xbegin + dx, roi.xend + dx, roi.ybegin + dy, roi.yend + dy
    );
}

ROI scale(ROI roi, float sx, float sy)
{
    int cx = (roi.xbegin + roi.xend) / 2;
    int cy = (roi.ybegin + roi.yend) / 2;

    int width = roi.width();
    int height = roi.height();
    int swidth = width * sx;
    int sheight = height * sy;

    int dx = (swidth - width) / 2;
    int dy = (sheight - height) / 2;
    
    ROI sroi = roi;
    {
        sroi.xbegin -= dx;
        sroi.xend = sroi.xbegin + swidth;
        sroi.ybegin -= dy;
        sroi.yend = sroi.ybegin + sheight;
    }
    return sroi;
}

ROI aspectratio(ROI roi, float aspectRatio)
{
    float ar = (float)roi.width() / roi.height();
    if (ar != aspectRatio)
    {
        int awidth, aheight;
        int wdiff = 0, hdiff = 0;
        if (ar < aspectRatio)
        {
            awidth =(int)(aspectRatio * roi.height());
            wdiff = awidth - roi.width();
        }
        else
        {
            aheight = (int)(roi.width() / aspectRatio);
            hdiff = aheight - roi.height();
        }
        int cx = roi.xbegin + roi.width() / 2;
        int cy = roi.ybegin + roi.height() / 2;

        ROI arroi = roi;
        {
            arroi.xbegin = cx - arroi.width() / 2 - wdiff / 2;
            arroi.xend = arroi.xbegin + arroi.width() + wdiff - wdiff / 2;
            arroi.ybegin = cy - arroi.height() / 2 - hdiff / 2;
            arroi.yend = arroi.ybegin + arroi.height() + hdiff - hdiff / 2;
        }
        return arroi;
    }
    return roi;
}

// utils - random
std::string random_by_length(int length)
{
    const std::string charset = "abcdefghijklmnopqrstuvwxyz123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distribution(0, charset.length() - 1);
    std::string name;
    for (int i = 0; i < length; ++i) {
        name.push_back(charset[distribution(gen)]);
    }
    return name;
}

// utils - files
std::string filename_by_extension(const std::string& filename, const std::string& extension) {
    size_t pos = filename.rfind('.');
    if (pos != std::string::npos) {
        std::string basename = filename.substr(0, pos);
        return basename + extension;
    }
    return filename;
}

std::string swapname_by_filename(const std::string& filename, const std::string& extension) {
    return(
        filename_by_extension(filename, "_" + random_by_length(5) + extension)
    );
}

bool remove_by_filename(const std::string& filename) {
    return std::remove(filename.c_str()) != 0;
}

// utils - fonts
std::string path_by_executable() {
    return(std::filesystem::current_path().string());
}

// utils - rawtoaces
bool rawtoaces(const std::string& filename, const std::string& outputname, int wbmethod, int matmethod, float headroom = 6.0f) {
    
    std::vector<const char*> settings;
    settings.push_back("rawtoaces");
    
    // white balance method
    settings.push_back("--wb-method");
    settings.push_back(std::to_string(wbmethod).c_str());
    
    // IDT matrix method
    settings.push_back("--mat-method");
    settings.push_back(std::to_string(matmethod).c_str());
    
    // highlight headroom
    settings.push_back("--headroom");
    settings.push_back(std::to_string(headroom).c_str());
    
    AcesRender& render = AcesRender::getInstance();
    putenv((char*)"TZ=UTC");

    render.initialize(pathsFinder());
    render.configureSettings(settings.size(), (char**)&settings[0]);

    render.preprocessRaw(filename.c_str());
    render.postprocessRaw();
    render.outputACES(outputname.c_str());
    return true;
}

// IT8 reference
struct IT8Data
{
    std::string id;
    float x;
    float y;
    float z;
    float l;
    float a;
    float b;
    float sd_x;
    float sd_y;
    float sd_z;
    float sd_de;
};

struct IT8File
{
    std::string originator;
    std::string descriptor;
    std::string created;
    std::string manufacturer;
    std::string prodDate;
    std::string serial;
    std::string material;
    int numberOfFields;
    std::vector<std::string> fieldNames;
    int numberOfSets;
    std::vector<IT8Data> data;
    bool valid = false;
    bool has_data_from_id(const std::string& id) const {
        for (const IT8Data& item : data) {
            if (item.id == id) {
                return true;
            }
        }
        return false;
    }
    const IT8Data find_data_from_id(const std::string& id) const {
        for (const IT8Data& item : data) {
            if (item.id == id) {
                return item;
            }
        }
        return IT8Data();
    }
};

IT8File openFile(const std::string& filename) {
    IT8File it8file;
    std::ifstream file(filename);
    if (!file.is_open()) {
        print_error("failed to open it8 reference file: ", filename);
        return it8file;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream iss(line);
        std::string key, value;
        std::getline(iss, key, ' ');
        std::getline(iss, value);
        key = nocr_from_str(key);
        if (key == "ORIGINATOR") {
            it8file.originator = value;
        } else if (key == "DESCRIPTOR") {
            it8file.descriptor = value;
        } else if (key == "CREATED") {
            it8file.created = value;
        } else if (key == "MANUFACTURER") {
            it8file.manufacturer = value;
        } else if (key == "PROD_DATE") {
            it8file.prodDate = value;
        } else if (key == "SERIAL") {
            it8file.serial = value;
        } else if (key == "MATERIAL") {
            it8file.material = value;
        } else if (key == "NUMBER_OF_FIELDS") {
            it8file.numberOfFields = std::stoi(value);
        } else if (key == "BEGIN_DATA_FORMAT") {
            std::getline(file, line);
            std::istringstream fieldNamesStream(line);
            std::string fieldName;
            while (fieldNamesStream >> fieldName) {
                it8file.fieldNames.push_back(fieldName);
            }
        } else if (key == "NUMBER_OF_SETS") {
            it8file.numberOfSets = std::stoi(value);
        } else if (key == "BEGIN_DATA") {
            it8file.data.reserve(it8file.numberOfSets);
            for (int i = 0; i < it8file.numberOfSets + 1; ++i) {
                std::getline(file, line);
                if (i) // skip field names
                {
                    std::istringstream dataStream(line);
                    IT8Data rowData;
                    if (!(dataStream >>
                          rowData.id >>
                          rowData.x >>
                          rowData.y >>
                          rowData.z >>
                          rowData.l >>
                          rowData.a >>
                          rowData.b >>
                          rowData.sd_x >>
                          rowData.sd_y >>
                          rowData.sd_z >>
                          rowData.sd_de))
                    {
                        print_error("could not parse data line: ", line);
                        break;
                    }
                    it8file.data.push_back(rowData);
                }
            }
        }
    }
    it8file.valid = true;
    file.close();
    return it8file;
}

// IT8 patches
struct IT8Patch
{
    std::string id;
    Imath::Vec3<float> measure_rgb;
    Imath::Vec3<float> measure_d65;
    Imath::Vec3<float> reference_rgb;
    Imath::Vec3<float> reference_lab;
    Imath::Vec3<float> reference_lab_d65;
    Imath::Vec3<float> reference_xyz;
    Imath::Vec3<float> reference_d50;
    Imath::Vec3<float> reference_d65;
    Imath::Vec3<float> diff_d65;
    bool valid = false;
};

struct IT8Dataset
{
    std::vector<IT8Patch> data;
};


// main
int 
main( int argc, const char * argv[])
{
    // Helpful for debugging to make sure that any crashes dump a stack
    // trace.
    Sysutil::setup_crash_stacktrace("stdout");
    
    // paths
    OIIO::attribute("font_searchpath",
        path_by_executable() +
        ":" +
        path_by_executable() + "/.."
    );
    
    // arguments
    Filesystem::convert_native_arguments(argc, (const char**)argv);
    ArgParse ap;

    ap.intro("it8tool -- a set of utilities for computation of color correction matrices from it8 charts\n");
    ap.usage("it8tool [options] filename...")
      .add_help(false)
      .exit_on_error(true);
    
    ap.separator("Commands that read chart:");
    ap.arg("-i %s:FILENAME")
      .help("Input file")
      .action(set_inputfile);
    
    ap.separator("General flags:");
    ap.arg("--help", &tool.help)
      .help("Print help message");
    
    ap.arg("-v", &tool.verbose)
      .help("Verbose status messages");
    
    ap.arg("-d", &tool.debug)
      .help("Debug status messages");
    
    ap.separator("Input flags:");
    ap.arg("--referencefile %s:IT8FILE", &tool.referencefile)
      .help("Input it8 reference file");
    
    ap.arg("--offset %s:OFFSET")
      .help("Input chart offset (default: 0.0, 0.0)")
      .action(set_offset);
    
    ap.arg("--scale %s:SCALE")
      .help("Input chart scale (default: 0.8, 0.8)")
      .action(set_scale);
    
    ap.arg("--rotate90")
      .help("Rotate chart 90 degrees")
      .action(set_rotate90);
    
    ap.arg("--rotate180")
      .help("Rotate chart 180 degrees")
      .action(set_rotate180);
    
    ap.arg("--rotate270")
      .help("Rotate chart 270 degrees")
      .action(set_rotate270);
    
    ap.separator("Output flags:");
    ap.arg("--outputfile %s:FILE", &tool.outputfile)
      .help("Output chart file");
    
    ap.arg("--outputdatafile %s:FILE", &tool.outputdatafile)
      .help("Output data file");
    
    ap.arg("--outputrawpatchfile %s:FILE", &tool.outputrawpatchfile)
      .help("Output raw patch file");
    
    ap.arg("--outputrawreferencefile %s:FILE", &tool.outputrawreferencefile)
      .help("Output raw reference file");
    
    ap.arg("--outputcalibrationmatrixfile %s:FILE", &tool.outputcalibrationmatrixfile)
      .help("Output calibration matrix file");
    
    ap.arg("--outputcalibrationlutfile %s:FILE", &tool.outputcalibrationlutfile)
      .help("Output calibration lut file");

    // clang-format on
    if (ap.parse_args(argc, (const char**)argv) < 0) {
        std::cerr << "error: " << ap.geterror() << std::endl;
        print_help(ap);
        ap.abort();
        return EXIT_FAILURE;
    }
    if (ap["help"].get<int>()) {
        print_help(ap);
        ap.abort();
        return EXIT_SUCCESS;
    }
    if (!tool.inputfile.size()) {
        std::cerr << "error: must have input filename\n";
        ap.briefusage();
        ap.abort();
        return EXIT_FAILURE;
    }
    if (!tool.referencefile.size()) {
        std::cerr << "error: must have reference file parameter\n";
        ap.briefusage();
        ap.abort();
        return EXIT_FAILURE;
    }
    if (argc <= 1) {
        ap.briefusage();
        std::cout << "\nFor detailed help: it8tool --help\n";
        return EXIT_FAILURE;
    }
    
    // it8 program
    std::cout << "it8tool -- a set of utilities for computation of color correction matrices from it8 charts" << std::endl;

    // it8 file
    IT8File file = openFile(tool.referencefile);
    if (file.valid) {
        print_info("Reading IT8 reference file: ", tool.referencefile);
        if (tool.debug) {
            print_info("IT8 attributes: ", "raw");
            print_info("  originator: ", file.originator);
            print_info("  descriptor: ", file.descriptor);
            print_info("  created: ", file.created);
            print_info("  manufacturer: ", file.manufacturer);
            print_info("  prod_date: ", file.prodDate);
            print_info("  serial: ", file.serial);
            print_info("  material: ", file.material);
            print_info("data: ", file.material);
            print_info("  patches: ", file.data.size());
        }
    } else {
        print_error("Failed when trying to read reference file, not an it8 file.", tool.referencefile);
        return EXIT_FAILURE;
    }
    
    // chart
    {
        print_info("Reading input file: ", tool.inputfile);
        std::string swapfile = swapname_by_filename(tool.inputfile, ".exr");
        if (rawtoaces(tool.inputfile, swapfile, 0, 1))
        {
            ImageBuf imagebuf = ImageBuf(swapfile);
            {
                // read
                if (!imagebuf.read(0, 0, TypeDesc::FLOAT)) {
                    print_error("Could not open image: ", tool.inputfile);
                    return EXIT_FAILURE;
                }
                
                // remove
                if (remove_by_filename(swapfile)) {
                    print_error("Could not remove swap: ", swapfile);
                    return EXIT_FAILURE;
                }
                
                // rotate
                if (tool.rotate90) {
                    imagebuf = ImageBufAlgo::rotate90(imagebuf);
                }

                if (tool.rotate180) {
                    imagebuf = ImageBufAlgo::rotate180(imagebuf);
                }
                
                if (tool.rotate270) {
                    imagebuf = ImageBufAlgo::rotate270(imagebuf);
                }
                
                ImageSpec& spec = imagebuf.specmod();
                int width = spec.width;
                int height = spec.height;
                int nchannels = spec.nchannels;
                
                // chart file
                if (tool.debug) {
                    print_info("Chart attributes: ", "raw");
                    print_info("  width: ", width);
                    print_info("  height: ", height);
                    print_info("  channels: ", nchannels);
                    print_info("  metadata: ", "raw");
                    for (size_t i = 0; i < spec.extra_attribs.size(); ++i) {
                        const ParamValue& attrib = spec.extra_attribs[i];
                        print_info(" name: ", attrib.name());
                        print_info("   type: ", attrib.type());
                        print_info("   value: " , attrib.get_string());
                    }
                }

                // dataset
                IT8Dataset dataset;
                
                // data region of interest
                ROI dataroi = spec.roi();
                
                // full region of interest
                ROI fullroi = spec.roi_full();
                
                // scale
                ROI chartroi = scale(fullroi, tool.xscale, tool.yscale);
                
                // offset
                chartroi = offset(chartroi, tool.xoffset, tool.yoffset);

                // color
                float color[] = { 1.0, 1.0, 1.0, 0.5 };
                
                // patches
                {
                    ROI roi = chartroi;
                    {
                        // 88% in vertical of the chart
                        roi.yend = chartroi.yend * 0.88;
                    }
                    ImageBufAlgo::render_box(
                        imagebuf,
                        roi.xbegin, roi.ybegin, roi.xend, roi.yend,
                        color,
                        false
                    );
                    
                    // aspect ratio 1.85 and scale
                    roi = aspectratio(roi, 1.85);
                    roi = scale(roi, 0.85, 0.85);
                    
                    ImageBufAlgo::render_box(
                        imagebuf,
                        roi.xbegin, roi.ybegin, roi.xend, roi.yend,
                        color,
                        false
                    );
                    // color patches pattern
                    {
                        int rows = 12;
                        int columns = 22;
                        
                        float cpwidth = roi.width() / columns;
                        float cpheight = roi.height() / rows;
                        
                        for (int i = 0; i < rows; ++i)
                        {
                            for (int j = 0; j < columns; ++j)
                            {
                                int xbegin = roi.xbegin + j * cpwidth;
                                int ybegin = roi.ybegin + i * cpheight;
                                int xend = xbegin + cpwidth;
                                int yend = ybegin + cpheight;
                                
                                ROI cproi = ROI(
                                    xbegin, xend, ybegin, yend
                                );
                                // scale
                                cproi = scale(cproi, 0.5, 0.5);
                                
                                // patch
                                IT8Patch patch;
                                
                                // id
                                patch.id =
                                    std::string(1, char_from_int(i+1)) +
                                    pad_from_int(2, j+1);
                                
                                // compute
                                std::vector<float> pixels(cproi.width() * cproi.height() * nchannels);
                                {
                                    imagebuf.set_roi_full(cproi);
                                    imagebuf.get_pixels(cproi, TypeDesc::FLOAT, pixels.data());
                                    
                                    // average
                                    patch.measure_rgb = Imath::Vec3<float>(0.0f, 0.0f, 0.0f);
                                    for (int c = 0; c < nchannels; ++c) {
                                        float sum = 0.0f;
                                        for (int i = c; i < pixels.size(); i += nchannels) {
                                            sum += pixels[i];
                                        }
                                        patch.measure_rgb[c] = sum / (cproi.width() * cproi.height());
                                    }
                                    
                                    // measure rgb
                                    print_info("Reading measure patch: ", patch.id);
                                    if (tool.debug) {
                                        print_info("Measure aces: ", "d65");
                                        print_info("  x aces rgb: ", std::to_string(patch.measure_rgb.x));
                                        print_info("  y aces rgb: ", std::to_string(patch.measure_rgb.y));
                                        print_info("  z aces rgb: ", std::to_string(patch.measure_rgb.z));
                                    }
            
                                    // measure d65
                                    patch.measure_d65 = ciexyzd65_from_aces2065_1(patch.measure_rgb);
                                    if (tool.debug) {
                                        print_info("Measure ciexyz: ", "d65");
                                        print_info("  x ciexyz: ", std::to_string(patch.measure_d65.x));
                                        print_info("  y ciexyz: ", std::to_string(patch.measure_d65.y));
                                        print_info("  z ciexyz: ", std::to_string(patch.measure_d65.z));
                                    }
                                    if (file.has_data_from_id(patch.id)) {
                                        
                                        IT8Data data = file.find_data_from_id(patch.id);
                                        print_info("Reading reference IT8: ", data.id);
                                        if (tool.debug) {
                                            print_info("IT8 reference: ", "raw");
                                            print_info("  x: ", std::to_string(data.x));
                                            print_info("  y: ", std::to_string(data.y));
                                            print_info("  z: ", std::to_string(data.z));
                                            print_info("  l: ", std::to_string(data.l));
                                            print_info("  a: ", std::to_string(data.a));
                                            print_info("  b: ", std::to_string(data.b));
                                            print_info("  sd_x: ", std::to_string(data.sd_x));
                                            print_info("  sd_y: ", std::to_string(data.sd_y));
                                            print_info("  sd_z: ", std::to_string(data.sd_z));
                                            print_info("  sd_de: ", std::to_string(data.sd_de));
                                        }
                                        // compute ciexyz
                                        {
                                            patch.reference_lab.x = data.l;
                                            patch.reference_lab.y = data.a;
                                            patch.reference_lab.z = data.b;
                                            if (tool.debug) {
                                                print_info("Reference lab: ", "raw");
                                                print_info("  x L: ", std::to_string(patch.reference_lab.x));
                                                print_info("  y a: ", std::to_string(patch.reference_lab.y));
                                                print_info("  z b: ", std::to_string(patch.reference_lab.z));
                                            }
                                            
                                            patch.reference_lab_d65 = ciexyzd65_from_lab(patch.reference_lab);
                                            if (tool.debug) {
                                                print_info("Reference lab ciexyz: ", "d65");
                                                print_info("  x ciexyz: ", std::to_string(patch.reference_lab_d65.x));
                                                print_info("  y ciexyz: ", std::to_string(patch.reference_lab_d65.y));
                                                print_info("  z ciexyz: ", std::to_string(patch.reference_lab_d65.z));
                                            }
                                            
                                            patch.reference_xyz.x = data.x / 100;
                                            patch.reference_xyz.y = data.y / 100;
                                            patch.reference_xyz.z = data.z / 100;
                                            
                                            patch.reference_d50 = Imath::Vec3<float>(
                                                patch.reference_xyz.x,
                                                patch.reference_xyz.y,
                                                patch.reference_xyz.z
                                            );
                                            if (tool.debug) {
                                                print_info("Reference ciexyz: ", "d50");
                                                print_info("  x ciexyz: ", std::to_string(patch.reference_d50.x));
                                                print_info("  y ciexyz: ", std::to_string(patch.reference_d50.y));
                                                print_info("  z ciexyz: ", std::to_string(patch.reference_d50.z));
                                            }
                                            patch.reference_d65 = ciexyzd65_from_D50(patch.reference_d50);
                                            if (tool.debug) {
                                                print_info("Reference ciexyz: ", "d65");
                                                print_info("  x ciexyz: ", std::to_string(patch.reference_d65.x));
                                                print_info("  y ciexyz: ", std::to_string(patch.reference_d65.y));
                                                print_info("  z ciexyz: ", std::to_string(patch.reference_d65.z));
                                                
                                            }
                                            patch.reference_rgb = aces2065_1_from_ciexyz_d65(patch.reference_d65);
                                            if (tool.debug) {
                                                print_info("Reference aces: ", "d65");
                                                print_info("  x aces rgb: ", std::to_string(patch.reference_rgb.x));
                                                print_info("  y aces rgb: ", std::to_string(patch.reference_rgb.y));
                                                print_info("  z aces rgb: ", std::to_string(patch.reference_rgb.z));
                                                
                                            }
                                            
                                            patch.diff_d65.x = patch.reference_d65.x - patch.measure_d65.x;
                                            patch.diff_d65.y = patch.reference_d65.y - patch.measure_d65.y;
                                            patch.diff_d65.z = patch.reference_d65.z - patch.measure_d65.z;
                                            if (tool.debug) {
                                                print_info("Diff ciexyz: ", "d65");
                                                print_info("  x ciexyz: ", std::to_string(patch.diff_d65.x));
                                                print_info("  y ciexyz: ", std::to_string(patch.diff_d65.y));
                                                print_info("  z ciexyz: ", std::to_string(patch.diff_d65.z));
                                                print_info("  x: ", percentage_from_float(patch.diff_d65.x / 1.0));
                                                print_info("  y: ", percentage_from_float(patch.diff_d65.y / 1.0));
                                                print_info("  z: ", percentage_from_float(patch.diff_d65.z / 1.0));
                                            }
                                            
                                            patch.valid = true;
                                            dataset.data.push_back(patch);
                                        }
                                    } else {
                                        print_error("could not find it8 data entry for, will be skipped: ", patch.id);
                                        
                                    }
                                    imagebuf.set_roi_full(fullroi);
                                }
                                
                                ImageBufAlgo::fill(
                                    imagebuf,
                                    { patch.measure_rgb.x, patch.measure_rgb.y, patch.measure_rgb.z },
                                    ROI(
                                      cproi.xbegin, cproi.xbegin + cproi.width() / 2.0, cproi.ybegin, cproi.yend
                                    )
                                );
                                
                                ImageBufAlgo::fill(
                                    imagebuf,
                                    { patch.reference_rgb.x, patch.reference_rgb.y, patch.reference_rgb.z },
                                    ROI(
                                      cproi.xbegin + cproi.width() / 2.0, cproi.xend, cproi.ybegin, cproi.yend
                                    )
                                );
                                
                                ImageBufAlgo::render_box(
                                    imagebuf,
                                    cproi.xbegin, cproi.ybegin, cproi.xend, cproi.yend,
                                    color,
                                    false
                                );

                                ImageBufAlgo::render_text(
                                    imagebuf,
                                    cproi.xbegin + cproi.width() / 2.0,
                                    cproi.ybegin + cproi.height() / 2.0,
                                    patch.id,
                                    40,
                                    "Roboto.ttf",
                                    color,
                                    ImageBufAlgo::TextAlignX::Center,
                                    ImageBufAlgo::TextAlignY::Center
                                );
                                
                                if (!patch.valid) {
                                    float color[] = { 1, 0, 0, 1 };
                                    ImageBufAlgo::render_line(
                                        imagebuf,
                                        cproi.xbegin,
                                        cproi.ybegin,
                                        cproi.xend,
                                        cproi.yend,
                                        color
                                    );
                                    ImageBufAlgo::render_line(
                                        imagebuf,
                                        cproi.xbegin+cproi.width(),
                                        cproi.ybegin,
                                        cproi.xbegin,
                                        cproi.ybegin+cproi.height(),
                                        color
                                    );
                                }
                            }
                        }
                    }
                    // grayscale patches
                    {
                        ROI gsroi = chartroi;
                        {
                            gsroi.ybegin = chartroi.yend * 0.88;
                        }
                        ImageBufAlgo::render_box(
                            imagebuf,
                            gsroi.xbegin, gsroi.ybegin, gsroi.xend, gsroi.yend,
                            color,
                            false
                        );
                        // grayscale pattern
                        {
                            int columns = 24;
                            float gpwidth = gsroi.width() / columns;
                            float gpheight = gsroi.height();
                            int gpcount = 0;
                            
                            for (int i = 0; i < columns; ++i)
                            {
                                int xbegin = gsroi.xbegin + i * gpwidth;
                                int ybegin = gsroi.ybegin;
                                int xend = xbegin + gpwidth;
                                int yend = ybegin + gpheight;

                                ROI gproi = ROI(
                                    xbegin, xend, ybegin, yend
                                );
                                // scale
                                gproi = scale(gproi, 0.5, 0.5);
                                
                                // patch
                                IT8Patch patch;
                                
                                // id
                                if (i == 0) {
                                    patch.id = "Dmin";
                                } else if (i == 23) {
                                    patch.id = "Dmax";
                                } else {
                                    patch.id = "GS" + pad_from_int(2, gpcount+1);
                                    gpcount++;
                                }

                                // compute
                                std::vector<float> pixels(gproi.width() * gproi.height() * nchannels);
                                {
                                    imagebuf.set_roi_full(gproi);
                                    imagebuf.get_pixels(gproi, TypeDesc::FLOAT, pixels.data());
                                    
                                    // average
                                    patch.measure_rgb = Imath::Vec3<float>(0.0f, 0.0f, 0.0f);
                                    for (int c = 0; c < nchannels; ++c) {
                                        float sum = 0.0f;
                                        for (int i = c; i < pixels.size(); i += nchannels) {
                                            sum += pixels[i];
                                        }
                                        patch.measure_rgb[c] = sum / (gproi.width() * gproi.height());
                                    }
                                    
                                    // measure rgb
                                    print_info("Reading measure patch: ", patch.id);
                                    if (tool.debug) {
                                        print_info("Measure aces: ", "d65");
                                        print_info("  x aces rgb: ", std::to_string(patch.measure_rgb.x));
                                        print_info("  y aces rgb: ", std::to_string(patch.measure_rgb.y));
                                        print_info("  z aces rgb: ", std::to_string(patch.measure_rgb.z));
                                    }
            
                                    // measure d65
                                    patch.measure_d65 = ciexyzd65_from_aces2065_1(patch.measure_rgb);
                                    if (tool.debug) {
                                        print_info("Measure ciexyz: ", "d65");
                                        print_info("  x ciexyz: ", std::to_string(patch.measure_d65.x));
                                        print_info("  y ciexyz: ", std::to_string(patch.measure_d65.y));
                                        print_info("  z ciexyz: ", std::to_string(patch.measure_d65.z));
                                    }
                                    if (file.has_data_from_id(patch.id)) {
                                        
                                        IT8Data data = file.find_data_from_id(patch.id);
                                        print_info("Reading reference IT8: ", data.id);
                                        if (tool.debug) {
                                            print_info("IT8 reference: ", "raw");
                                            print_info("  x: ", std::to_string(data.x));
                                            print_info("  y: ", std::to_string(data.y));
                                            print_info("  z: ", std::to_string(data.z));
                                            print_info("  l: ", std::to_string(data.l));
                                            print_info("  a: ", std::to_string(data.a));
                                            print_info("  b: ", std::to_string(data.b));
                                            print_info("  sd_x: ", std::to_string(data.sd_x));
                                            print_info("  sd_y: ", std::to_string(data.sd_y));
                                            print_info("  sd_z: ", std::to_string(data.sd_z));
                                            print_info("  sd_de: ", std::to_string(data.sd_de));
                                        }
                                        // compute ciexyz
                                        {
                                            patch.reference_lab.x = data.l;
                                            patch.reference_lab.y = data.a;
                                            patch.reference_lab.z = data.b;
                                            if (tool.debug) {
                                                print_info("Reference lab: ", "raw");
                                                print_info("  x L: ", std::to_string(patch.reference_lab.x));
                                                print_info("  y a: ", std::to_string(patch.reference_lab.y));
                                                print_info("  z b: ", std::to_string(patch.reference_lab.z));
                                            }
                                            
                                            patch.reference_lab_d65 = ciexyzd65_from_lab(patch.reference_lab);
                                            if (tool.debug) {
                                                print_info("Reference lab ciexyz: ", "d65");
                                                print_info("  x ciexyz: ", std::to_string(patch.reference_lab_d65.x));
                                                print_info("  y ciexyz: ", std::to_string(patch.reference_lab_d65.y));
                                                print_info("  z ciexyz: ", std::to_string(patch.reference_lab_d65.z));
                                            }
                                            
                                            patch.reference_xyz.x = data.x / 100;
                                            patch.reference_xyz.y = data.y / 100;
                                            patch.reference_xyz.z = data.z / 100;
                                            
                                            patch.reference_d50 = Imath::Vec3<float>(
                                                patch.reference_xyz.x,
                                                patch.reference_xyz.y,
                                                patch.reference_xyz.z
                                            );
                                            if (tool.debug) {
                                                print_info("Reference ciexyz: ", "d50");
                                                print_info("  x ciexyz: ", std::to_string(patch.reference_d50.x));
                                                print_info("  y ciexyz: ", std::to_string(patch.reference_d50.y));
                                                print_info("  z ciexyz: ", std::to_string(patch.reference_d50.z));
                                            }
                                            
                                            patch.reference_d65 = ciexyzd65_from_D50(patch.reference_d50);
                                            if (tool.debug) {
                                                print_info("Reference ciexyz: ", "d65");
                                                print_info("  x ciexyz: ", std::to_string(patch.reference_d65.x));
                                                print_info("  y ciexyz: ", std::to_string(patch.reference_d65.y));
                                                print_info("  z ciexyz: ", std::to_string(patch.reference_d65.z));
                                                
                                            }
                                            patch.reference_rgb = aces2065_1_from_ciexyz_d65(patch.reference_d65);
                                            if (tool.debug) {
                                                print_info("Reference aces: ", "d65");
                                                print_info("  x aces rgb: ", std::to_string(patch.reference_rgb.x));
                                                print_info("  y aces rgb: ", std::to_string(patch.reference_rgb.y));
                                                print_info("  z aces rgb: ", std::to_string(patch.reference_rgb.z));
                                                
                                            }
                                            
                                            patch.diff_d65.x = patch.reference_d65.x - patch.measure_d65.x;
                                            patch.diff_d65.y = patch.reference_d65.y - patch.measure_d65.y;
                                            patch.diff_d65.z = patch.reference_d65.z - patch.measure_d65.z;
                                            if (tool.debug) {
                                                print_info("Diff ciexyz: ", "d65");
                                                print_info("  x ciexyz: ", std::to_string(patch.diff_d65.x));
                                                print_info("  y ciexyz: ", std::to_string(patch.diff_d65.y));
                                                print_info("  z ciexyz: ", std::to_string(patch.diff_d65.z));
                                                print_info("  x: ", percentage_from_float(patch.diff_d65.x / 1.0));
                                                print_info("  y: ", percentage_from_float(patch.diff_d65.y / 1.0));
                                                print_info("  z: ", percentage_from_float(patch.diff_d65.z / 1.0));
                                            }

                                            patch.valid = true;
                                            dataset.data.push_back(patch);
                                        }
                                    } else {
                                        print_error("could not find it8 data entry for, will be skipped: ", patch.id);
                                        
                                    }
                                    imagebuf.set_roi_full(fullroi);
                                }
                                
                                ImageBufAlgo::fill(
                                    imagebuf,
                                    { patch.measure_rgb.x, patch.measure_rgb.y, patch.measure_rgb.z },
                                    ROI(
                                      gproi.xbegin, gproi.xbegin + gproi.width() / 2.0, gproi.ybegin, gproi.yend
                                    )
                                );
                                
                                ImageBufAlgo::fill(
                                    imagebuf,
                                    { patch.reference_rgb.x, patch.reference_rgb.y, patch.reference_rgb.z },
                                    ROI(
                                      gproi.xbegin + gproi.width() / 2.0, gproi.xend, gproi.ybegin, gproi.yend
                                    )
                                );
                                
                                ImageBufAlgo::render_box(
                                    imagebuf,
                                    gproi.xbegin, gproi.ybegin, gproi.xend, gproi.yend,
                                    color,
                                    false
                                );

                                ImageBufAlgo::render_text(
                                    imagebuf,
                                    gproi.xbegin + gproi.width() / 2.0,
                                    gproi.ybegin + gproi.height() / 2.0,
                                    patch.id,
                                    40,
                                    "Roboto.ttf",
                                    color,
                                    ImageBufAlgo::TextAlignX::Center,
                                    ImageBufAlgo::TextAlignY::Center
                                );
                                
                                if (!patch.valid) {
                                    float color[] = { 1, 0, 0, 1 };
                                    ImageBufAlgo::render_line(
                                        imagebuf,
                                        gproi.xbegin,
                                        gproi.ybegin,
                                        gproi.xend,
                                        gproi.yend,
                                        color
                                    );
                                    ImageBufAlgo::render_line(
                                        imagebuf,
                                        gproi.xbegin+gproi.width(),
                                        gproi.ybegin,
                                        gproi.xbegin,
                                        gproi.ybegin+gproi.height(),
                                        color
                                    );
                                }
                            }
                        }
                    }
                }
                
                // color calibration matrix
                Eigen::Matrix<float, Eigen::Dynamic, 3> calmeasures;
                Eigen::Matrix<float, Eigen::Dynamic, 3> calreferences;
                calmeasures.resize(dataset.data.size(), 3);
                calreferences.resize(dataset.data.size(), 3);
                
                for (int i = 0; i < dataset.data.size(); i++) {
                    IT8Patch patch = dataset.data[i];
                    for (int j = 0; j < 3; j++) {
                        calmeasures.coeffRef(i, j) = patch.measure_rgb[j];
                        calreferences.coeffRef(i, j) = patch.reference_rgb[j];
                    }
                }

                auto transpose = calmeasures.transpose();
                auto inverse = (transpose * calmeasures).inverse() * transpose;
                Eigen::Matrix<float, 3, 3> matrix = (inverse * calreferences).transpose(); // eigen row-order to column-order convention
                
                Imath::Matrix33<float> ccmmatrix;
                for (int row = 0; row < 3; ++row) {
                    for (int col = 0; col < 3; ++col) {
                        ccmmatrix[row][col] = matrix(row, col);
                    }
                }
                
                std::stringstream ccmstream;
                ccmstream << ccmmatrix[0][0] << " " << ccmmatrix[0][1] << " " << ccmmatrix[0][2] << " "
                          << ccmmatrix[1][0] << " " << ccmmatrix[1][1] << " " << ccmmatrix[1][2] << " "
                          << ccmmatrix[2][0] << " " << ccmmatrix[2][1] << " " << ccmmatrix[2][2];
                
                if (tool.debug) {
                    print_info("Color calibration matrix: ", ccmstream.str());
                    for (int i = 0; i < dataset.data.size(); i++) {
                        IT8Patch patch = dataset.data[i];
                        
                        Imath::Vec3<float> measure = patch.measure_rgb;
                        Imath::Vec3<float> reference = patch.reference_rgb;
                        
                        print_info("Reading measure patch: ", patch.id);
                        print_info(" measure aces rgb: ", measure);
                        print_info(" reference aces rgb: ", reference);
                        
                        Imath::Vec3<float> calresult = measure * ccmmatrix.transposed(); // multiply in imath row-order
                        print_info("Correction: ", "d65");
                        print_info("  x: ", percentage_from_float(reference.x - calresult.x));
                        print_info("  y: ", percentage_from_float(reference.y - calresult.y));
                        print_info("  z: ", percentage_from_float(reference.z - calresult.z));
                    }
                }
                
                
                // datafile
                if (tool.outputdatafile.length())
                {
                    print_info("Writing it8 data file: ", tool.outputdatafile);
                    std::ofstream outputFile(tool.outputdatafile);
                    if (outputFile.is_open()) {
                        outputFile << ", "
                                   << "mrgb.x, mrgb.y, mrgb.z, md65.x, md65.y, md65.z, "
                                   << "reflab.x, reflab.y, reflab.z, reflabd65.x, reflabd65.y, reflabd65.z, refxyz.x, refxyz.y, refxyz.z, refd50.x, refd50.y, refd50.z, refd65.x, refd65.y, refd65.z, "
                                   << "diffd65.x, diffd65.y, diffd65.z, "
                                   << "refrgb.x, refrgb.y, refrgb.z"
                                   << std::endl;
                        
                        for (IT8Patch patch : dataset.data) {
                            outputFile << patch.id
                                       << ", " << patch.measure_rgb.x
                                       << ", " << patch.measure_rgb.y
                                       << ", " << patch.measure_rgb.z
                                       << ", " << patch.measure_d65.x
                                       << ", " << patch.measure_d65.y
                                       << ", " << patch.measure_d65.z
                                       << ", " << patch.reference_lab.x
                                       << ", " << patch.reference_lab.y
                                       << ", " << patch.reference_lab.z
                                       << ", " << patch.reference_lab_d65.x
                                       << ", " << patch.reference_lab_d65.y
                                       << ", " << patch.reference_lab_d65.z
                                       << ", " << patch.reference_xyz.x
                                       << ", " << patch.reference_xyz.y
                                       << ", " << patch.reference_xyz.z
                                       << ", " << patch.reference_d50.x
                                       << ", " << patch.reference_d50.y
                                       << ", " << patch.reference_d50.z
                                       << ", " << patch.reference_d65.x
                                       << ", " << patch.reference_d65.y
                                       << ", " << patch.reference_d65.z
                                       << ", " << patch.reference_rgb.x
                                       << ", " << patch.reference_rgb.y
                                       << ", " << patch.reference_rgb.z
                                       << ", " << patch.diff_d65.x
                                       << ", " << patch.diff_d65.y
                                       << ", " << patch.diff_d65.z
                                       << std::endl;
                        }
                    } else {
                        print_error("could not open output data file: ", tool.outputdatafile);
                    }
                    outputFile.close();
                }
                // raw patch file
                if (tool.outputrawpatchfile.length())
                {
                    print_info("Writing it8 raw patch file: ", tool.outputrawpatchfile);
                    std::ofstream outputFile(tool.outputrawpatchfile);
                    if (outputFile.is_open()) {
                        outputFile << ", x, y, z" << std::endl;
                        for (IT8Patch patch : dataset.data) {
                            outputFile << patch.id
                                       << ", " << patch.measure_d65.x
                                       << ", " << patch.measure_d65.y
                                       << ", " << patch.measure_d65.z
                                       << std::endl;
                        }
                     } else {
                         print_error("could not open output raw patch file: ", tool.outputrawpatchfile);
                     }
                     outputFile.close();
                }
                // raw reference file
                if (tool.outputrawreferencefile.length())
                {
                    print_info("Writing it8 raw reference file: ", tool.outputrawreferencefile);
                    std::ofstream outputFile(tool.outputrawreferencefile);
                    if (outputFile.is_open()) {
                        outputFile << ", x, y, z" << std::endl;
                        for (IT8Patch patch : dataset.data) {
                            outputFile << patch.id
                                       << ", " << patch.reference_d65.x
                                       << ", " << patch.reference_d65.y
                                       << ", " << patch.reference_d65.z
                                       << std::endl;
                        }
                     } else {
                         print_error("could not open output raw reference file: ", tool.outputrawreferencefile);
                     }
                     outputFile.close();
                }
                
                // calibration matrix file
                if (tool.outputcalibrationmatrixfile.length())
                {
                    print_info("Writing it8 calibration matrix file: ", tool.outputcalibrationmatrixfile);
                    std::ofstream outputFile(tool.outputcalibrationmatrixfile);
                    if (outputFile.is_open()) {
                        outputFile << ccmmatrix[0][0] << ", " << ccmmatrix[0][1] << ", " << ccmmatrix[0][2] << std::endl
                                   << ccmmatrix[1][0] << ", " << ccmmatrix[1][1] << ", " << ccmmatrix[1][2] << std::endl
                                   << ccmmatrix[2][0] << ", " << ccmmatrix[2][1] << ", " << ccmmatrix[2][2] << std::endl;
                        
                    } else {
                        print_error("could not open output calibration matrix file: ", tool.outputcalibrationmatrixfile);
                    }
                    outputFile.close();
                }
                
                if (tool.outputcalibrationlutfile.length())
                {
                    print_info("Writing output calibration matrix lut: ", tool.outputcalibrationlutfile);
                    
                    int size = 32;
                    int nsize = size * size * size;
                    std::vector<float> values;
                    
                    for (unsigned i = 0; i < nsize; ++i)
                    {
                        float r = std::max(0.0f, std::min(1.0f, static_cast<float>(i % size) / (size - 1)));
                        float g = std::max(0.0f, std::min(1.0f, static_cast<float>((i / size) % size) / (size - 1)));
                        float b = std::max(0.0f, std::min(1.0f, static_cast<float>((i / (size * size)) % size) / (size - 1)));

                        Imath::Vec3<float> rgb(r, g, b);
                        rgb = rgb * ccmmatrix.transposed();

                        values.push_back(rgb.x);
                        values.push_back(rgb.y);
                        values.push_back(rgb.z);
                    }

                    std::ofstream outputFile(tool.outputcalibrationlutfile);
                    if (outputFile)
                    {
                        outputFile << "# IT8Tool Color Calibration LUT" << std::endl;
                        outputFile << "#   Input: Aces2065-1 AP0 Linear with D65 white point" << std::endl;
                        outputFile << "#        : floating point data (range 0.0 - 1.0)" << std::endl;
                        outputFile << "#  Output: Aces2065-1 AP0 Linear with D65 white point" << std::endl;
                        outputFile << "#        : floating point data (range 0.0 - 1.0)" << std::endl;
                        outputFile << std::endl;
                        outputFile << "LUT_3D_SIZE " << size << std::endl;
                        outputFile << "DOMAIN_MIN 0.0 0.0 0.0" << std::endl;
                        outputFile << "DOMAIN_MAX 1.0 1.0 1.0" << std::endl;
                        outputFile << std::endl;
                        
                        for (unsigned i = 0; i < nsize; ++i) {
                            outputFile << values[i * 3] << " "
                                       << values[i * 3 + 1] << " "
                                       << values[i * 3 + 2] << std::endl;
                        }
                        outputFile.close();
                        
                    } else {
                        print_error("could not open output calibration lut file: ", tool.outputcalibrationlutfile);
                    }
                }

                if (tool.outputfile.size()) {
                    print_info("Writing it8 output file: ", tool.outputfile);
                    
                    if (!imagebuf.write(tool.outputfile)) {
                        print_error("could not write output file", imagebuf.geterror());
                    }
                }
            }
            
        } else {
            print_error("Failed when trying to read chart file, not a valid file.", tool.inputfile);
            return EXIT_FAILURE;
        }
    }
    return 0;
}
