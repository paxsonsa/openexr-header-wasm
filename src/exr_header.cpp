#include "OpenEXR/ImfNamespace.h"
#include <OpenEXR/ImfMultiPartInputFile.h>
#include <OpenEXR/ImfBoxAttribute.h>
#include <OpenEXR/ImfChannelListAttribute.h>
#include <OpenEXR/ImfChromaticitiesAttribute.h>
#include <OpenEXR/ImfCompressionAttribute.h>
#include <OpenEXR/ImfDoubleAttribute.h>
#include <OpenEXR/ImfEnvmapAttribute.h>
#include <OpenEXR/ImfFloatAttribute.h>
#include <OpenEXR/ImfIntAttribute.h>
#include <OpenEXR/ImfKeyCodeAttribute.h>
#include <OpenEXR/ImfLineOrderAttribute.h>
#include <OpenEXR/ImfMatrixAttribute.h>
#include <OpenEXR/ImfPreviewImageAttribute.h>
#include <OpenEXR/ImfRationalAttribute.h>
#include <OpenEXR/ImfStringAttribute.h>
#include <OpenEXR/ImfStringVectorAttribute.h>
#include <OpenEXR/ImfTileDescriptionAttribute.h>
#include <OpenEXR/ImfTimeCodeAttribute.h>
#include <OpenEXR/ImfVecAttribute.h>
#include <OpenEXR/ImfVersion.h>
#include <OpenEXR/ImfHeader.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <emscripten/bind.h>


using namespace OPENEXR_IMF_NAMESPACE;
using namespace std;


void
printCompression (Compression c, iostream &output)
{
    switch (c)
    {
        case NO_COMPRESSION:
            output << "none";
            break;

        case RLE_COMPRESSION:
            output << "run-length encoding";
            break;

        case ZIPS_COMPRESSION:
            output << "zip, individual scanlines";
            break;

        case ZIP_COMPRESSION:
            output << "zip, multi-scanline blocks";
            break;

        case PIZ_COMPRESSION:
            output << "piz";
            break;

        case PXR24_COMPRESSION:
            output << "pxr24";
            break;

        case B44_COMPRESSION:
            output << "b44";
            break;

        case B44A_COMPRESSION:
            output << "b44a";
            break;

        case DWAA_COMPRESSION:
            output << "dwa, small scanline blocks";
            break;

        case DWAB_COMPRESSION:
            output << "dwa, medium scanline blocks";
            break;

        default:
            output << int (c);
            break;
    }
}


void
printLineOrder (LineOrder lo, iostream &output)
{
    switch (lo)
    {
        case INCREASING_Y:
            output << "increasing y";
            break;

        case DECREASING_Y:
            output << "decreasing y";
            break;

        case RANDOM_Y:
            output << "random y";
            break;

        default:
            output << int (lo);
            break;
    }
}


void
printPixelType (PixelType pt, iostream &output)
{
    switch (pt)
    {
        case UINT:
            output << "32-bit unsigned integer";
            break;

        case HALF:
            output << "16-bit floating-point";
            break;

        case FLOAT:
            output << "32-bit floating-point";
            break;

        default:
            output << "type " << int (pt);
            break;
    }
}


void
printLevelMode (LevelMode lm, iostream &output)
{
    switch (lm)
    {
        case ONE_LEVEL:
            output << "single level";
            break;

        case MIPMAP_LEVELS:
            output << "mip-map";
            break;

        case RIPMAP_LEVELS:
            output << "rip-map";
            break;

        default:
            output << "level mode " << int (lm);
            break;
    }
}


void
printLevelRoundingMode (LevelRoundingMode lm, iostream &output)
{
    switch (lm)
    {
        case ROUND_DOWN:
            output << "down";
            break;

        case ROUND_UP:
            output << "up";
            break;

        default:
            output << "mode " << int (lm);
            break;
    }
}


void
printTimeCode (TimeCode tc, iostream &output)
{
    output << "    "
    "time " <<
    setfill ('0') <<
#ifndef HAVE_COMPLETE_IOMANIP
    setw (2) << tc.hours() << ":" <<
    setw (2) << tc.minutes() << ":" <<
    setw (2) << tc.seconds() << ":" <<
    setw (2) << tc.frame() << "\n" <<
#else
    setw (2) << right << tc.hours() << ":" <<
    setw (2) << right << tc.minutes() << ":" <<
    setw (2) << right << tc.seconds() << ":" <<
    setw (2) << right << tc.frame() << "\n" <<
#endif
    setfill (' ') <<
    "    "
    "drop frame " << tc.dropFrame() << ", "
    "color frame " << tc.colorFrame() << ", "
    "field/phase " << tc.fieldPhase() << "\n"
    "    "
    "bgf0 " << tc.bgf0() << ", "
    "bgf1 " << tc.bgf1() << ", "
    "bgf2 " << tc.bgf2() << "\n"
    "    "
    "user data 0x" << hex << tc.userData() << dec;
}


void
printEnvmap (Envmap e, iostream &output)
{
    switch (e)
    {
        case ENVMAP_LATLONG:
            output << "latitude-longitude map";
            break;

        case ENVMAP_CUBE:
            output << "cube-face map";
            break;

        default:
            output << "map type " << int (e);
            break;
    }
}


void
printChannelList (const ChannelList &cl, iostream &output)
{
    for (ChannelList::ConstIterator i = cl.begin(); i != cl.end(); ++i)
    {
        output << "\n    " << i.name() << ", ";

        printPixelType(i.channel().type, output);

        output << ", sampling " <<
        i.channel().xSampling << " " <<
        i.channel().ySampling;

        if (i.channel().pLinear)
            output << ", plinear";
    }
}


void printInfo (const char fileName[], iostream &output)
{
    MultiPartInputFile in (fileName);
    int parts = in.parts();

    //
    // Check to see if any parts are incomplete
    //

    bool fileComplete = true;

    for (int i = 0; i < parts && fileComplete; ++i)
        if (!in.partComplete (i))
            fileComplete = false;

    //
    // Print file name and file format version
    //

    output << "\nfile " << fileName <<
            (fileComplete? "": " (incomplete)") <<
            ":\n\n";

    output << "file format version: " <<
            getVersion (in.version()) << ", "
            "flags 0x" <<
            setbase (16) << getFlags (in.version()) << setbase (10) << "\n";

    //
    // Print the header of every part in the file
    //

    for (int p = 0; p < parts ; ++p)
    {
        const Header & h = in.header (p);

        if (parts != 1)
        {
            output  << "\n\n part " << p <<
            (in.partComplete (p)? "": " (incomplete)") <<
            ":\n";

        }

        for (Header::ConstIterator i = h.begin(); i != h.end(); ++i)
        {
            const Attribute *a = &i.attribute();
            output << i.name() << " (type " << a->typeName() << ")";

            if (const Box2iAttribute *ta =
                            dynamic_cast <const Box2iAttribute *> (a))
            {
                output << ": " << ta->value().min << " - " << ta->value().max;
            }

            else if (const Box2fAttribute *ta =
                            dynamic_cast <const Box2fAttribute *> (a))
            {
                output << ": " << ta->value().min << " - " << ta->value().max;
            }
            else if (const ChannelListAttribute *ta =
                            dynamic_cast <const ChannelListAttribute *> (a))
            {
                output << ":";
                printChannelList (ta->value(), output);
            }
            else if (const ChromaticitiesAttribute *ta =
                            dynamic_cast <const ChromaticitiesAttribute *> (a))
            {
                output << ":\n"
                "    red   " << ta->value().red << "\n"
                "    green " << ta->value().green << "\n"
                "    blue  " << ta->value().blue << "\n"
                "    white " << ta->value().white;
            }
            else if (const CompressionAttribute *ta =
                            dynamic_cast <const CompressionAttribute *> (a))
            {
                output << ": ";
                printCompression (ta->value(), output);
            }
            else if (const DoubleAttribute *ta =
                            dynamic_cast <const DoubleAttribute *> (a))
            {
                output << ": " << ta->value();
            }
            else if (const EnvmapAttribute *ta =
                            dynamic_cast <const EnvmapAttribute *> (a))
            {
                output << ": ";
                printEnvmap (ta->value(), output);
            }
            else if (const FloatAttribute *ta =
                            dynamic_cast <const FloatAttribute *> (a))
            {
                output << ": " << ta->value();
            }
            else if (const IntAttribute *ta =
                            dynamic_cast <const IntAttribute *> (a))
            {
                output << ": " << ta->value();
            }
            else if (const KeyCodeAttribute *ta =
                            dynamic_cast <const KeyCodeAttribute *> (a))
            {
                output << ":\n"
                "    film manufacturer code " <<
                ta->value().filmMfcCode() << "\n"
                "    film type code " <<
                ta->value().filmType() << "\n"
                "    prefix " <<
                ta->value().prefix() << "\n"
                "    count " <<
                ta->value().count() << "\n"
                "    perf offset " <<
                ta->value().perfOffset() << "\n"
                "    perfs per frame " <<
                ta->value().perfsPerFrame() << "\n"
                "    perfs per count " <<
                ta->value().perfsPerCount();
            }
            else if (const LineOrderAttribute *ta =
                            dynamic_cast <const LineOrderAttribute *> (a))
            {
                output << ": ";
                printLineOrder (ta->value(), output);
            }
            else if (const M33fAttribute *ta =
                            dynamic_cast <const M33fAttribute *> (a))
            {
                output << ":\n"
                "   (" <<
                ta->value()[0][0] << " " <<
                ta->value()[0][1] << " " <<
                ta->value()[0][2] << "\n    " <<
                ta->value()[1][0] << " " <<
                ta->value()[1][1] << " " <<
                ta->value()[1][2] << "\n    " <<
                ta->value()[2][0] << " " <<
                ta->value()[2][1] << " " <<
                ta->value()[2][2] << ")";
            }
            else if (const M44fAttribute *ta =
                            dynamic_cast <const M44fAttribute *> (a))
            {
                output << ":\n"
                "   (" <<
                ta->value()[0][0] << " " <<
                ta->value()[0][1] << " " <<
                ta->value()[0][2] << " " <<
                ta->value()[0][3] << "\n    " <<
                ta->value()[1][0] << " " <<
                ta->value()[1][1] << " " <<
                ta->value()[1][2] << " " <<
                ta->value()[1][3] << "\n    " <<
                ta->value()[2][0] << " " <<
                ta->value()[2][1] << " " <<
                ta->value()[2][2] << " " <<
                ta->value()[2][3] << "\n    " <<
                ta->value()[3][0] << " " <<
                ta->value()[3][1] << " " <<
                ta->value()[3][2] << " " <<
                ta->value()[3][3] << ")";
            }
            else if (const PreviewImageAttribute *ta =
                            dynamic_cast <const PreviewImageAttribute *> (a))
            {
                output << ": " <<
                ta->value().width()  << " by " <<
                ta->value().height() << " pixels";
            }
            else if (const StringAttribute *ta =
                            dynamic_cast <const StringAttribute *> (a))
            {
                output << ": \"" << ta->value() << "\"";
            }
            else if (const StringVectorAttribute * ta =
                            dynamic_cast<const StringVectorAttribute *>(a))
            {
                output << ":";

                for (StringVector::const_iterator i = ta->value().begin();
                                i != ta->value().end();
                                ++i)
                {
                    output << "\n    \"" << *i << "\"";
                }
            }
            else if (const RationalAttribute *ta =
                            dynamic_cast <const RationalAttribute *> (a))
            {
                output << ": " << ta->value().n << "/" << ta->value().d <<
                " (" << double (ta->value()) << ")";
            }
            else if (const TileDescriptionAttribute *ta =
                            dynamic_cast <const TileDescriptionAttribute *> (a))
            {
                output << ":\n    ";

                printLevelMode (ta->value().mode, output);

                output << "\n    tile size " <<
                ta->value().xSize << " by " <<
                ta->value().ySize << " pixels";

                if (ta->value().mode != ONE_LEVEL)
                {
                    output << "\n    level sizes rounded ";
                    printLevelRoundingMode (ta->value().roundingMode, output);
                }
            }
            else if (const TimeCodeAttribute *ta =
                            dynamic_cast <const TimeCodeAttribute *> (a))
            {
                output << ":\n";
                printTimeCode (ta->value(), output);
            }
            else if (const V2iAttribute *ta =
                            dynamic_cast <const V2iAttribute *> (a))
            {
                output << ": " << ta->value();
            }
            else if (const V2fAttribute *ta =
                            dynamic_cast <const V2fAttribute *> (a))
            {
                output << ": " << ta->value();
            }
            else if (const V3iAttribute *ta =
                            dynamic_cast <const V3iAttribute *> (a))
            {
                output << ": " << ta->value();
            }
            else if (const V3fAttribute *ta =
                            dynamic_cast <const V3fAttribute *> (a))
            {
                output << ": " << ta->value();
            }

            output << '\n';
        }
    }

    output << endl;
}

std::string getHeaderInfo(std::string fileName) {
    stringstream message;
    printInfo(fileName.c_str(), message);
    return message.str();
}

EMSCRIPTEN_BINDINGS(my_module) {
    emscripten::function("getHeaderInfo", &getHeaderInfo);
}
