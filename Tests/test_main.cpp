#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include "../Include/VectorTile.h"
#include "../Include/MBTileReader.h"
#include "../cppGzip/DecodeGzip.h"
#include "../cppGzip/EncodeGzip.h"
using namespace std;

// ---- Minimal test framework ----

static int failures = 0;
static int total = 0;

#define CHECK(expr) do { \
    total++; \
    if (!(expr)) { \
        cerr << "  FAIL [line " << __LINE__ << "]: " << #expr << "\n"; \
        failures++; \
    } \
} while(0)

#define CHECK_NEAR(a, b, tol) do { \
    total++; \
    double _a = (double)(a), _b = (double)(b), _t = (double)(tol); \
    if (fabs(_a - _b) > _t) { \
        cerr << "  FAIL [line " << __LINE__ << "]: |" << #a << " - " << #b \
             << "| = " << fabs(_a - _b) << " > " << _t << "\n"; \
        failures++; \
    } \
} while(0)

#define CHECK_THROWS(expr) do { \
    total++; \
    bool threw = false; \
    try { expr; } catch (...) { threw = true; } \
    if (!threw) { \
        cerr << "  FAIL [line " << __LINE__ << "]: expected exception: " << #expr << "\n"; \
        failures++; \
    } \
} while(0)

// ---- Capturing DecodeVectorTileResults ----

struct CapturedFeature {
    int type;
    bool hasId;
    unsigned long long id;
    map<string, string> tags;
    vector<Point2D> points;
    vector<vector<Point2D>> lines;
    vector<Polygon2D> polygons;
};

struct CapturedLayer {
    string name;
    int version, extent;
    vector<CapturedFeature> features;
};

class CapturingResults : public DecodeVectorTileResults {
public:
    vector<CapturedLayer> layers;
    CapturedLayer *current = nullptr;

    void NumLayers(int) override {}
    void LayerStart(const char *name, int version, int extent) override {
        layers.push_back({name, version, extent, {}});
        current = &layers.back();
    }
    void LayerEnd() override { current = nullptr; }
    void Feature(int type, bool hasId, unsigned long long id,
                 const map<string, string> &tags,
                 vector<Point2D> &pts,
                 vector<vector<Point2D>> &lines,
                 vector<Polygon2D> &polys) override {
        current->features.push_back({type, hasId, id, tags, pts, lines, polys});
    }
    void Finish() override {}
};

// ---- Tests ----

void test_gzip_roundtrip()
{
    string input = "Hello, world! This is a test string for gzip round-trip verification.";
    string compressed, decompressed;
    EncodeGzipQuick(input, compressed);
    DecodeGzipQuick(compressed, decompressed);
    CHECK(decompressed == input);
}

void test_gzip_empty()
{
    string input, compressed, decompressed;
    EncodeGzipQuick(input, compressed);
    DecodeGzipQuick(compressed, decompressed);
    CHECK(decompressed == input);
}

void test_gzip_large()
{
    // 256 KB with a non-trivial pattern to exercise buffering
    string input(256 * 1024, '\0');
    for (size_t i = 0; i < input.size(); i++)
        input[i] = (char)(i % 251);
    string compressed, decompressed;
    EncodeGzipQuick(input, compressed);
    DecodeGzipQuick(compressed, decompressed);
    CHECK(decompressed == input);
    CHECK(compressed.size() < input.size());
}

void test_coord_conversion_roundtrip()
{
    // lon round-trip: tilex2long returns the left edge of tile x, which maps
    // back exactly (no floating-point ambiguity).
    for (int z = 0; z <= 12; z++) {
        int n = 1 << z;
        for (int x = 0; x < n && x < 16; x++)
            CHECK(long2tilex(tilex2long(x, z), z) == x);
    }

    // lat round-trip: tiley2lat returns a tile *edge*, and an exact boundary
    // latitude is ambiguous under floor — lat2tiley can land one tile off.
    // Test with tile centres (average of top and bottom edge) instead.
    for (int z = 0; z <= 12; z++) {
        double n = (double)(1 << z);
        int ni = (int)n;
        for (int y_tms = 0; y_tms < ni && y_tms < 16; y_tms++) {
            int y_xyz = ni - y_tms - 1;
            double latTop = atan(sinh(M_PI * (1.0 - 2.0 *  y_xyz      / n))) * 180.0 / M_PI;
            double latBot = atan(sinh(M_PI * (1.0 - 2.0 * (y_xyz + 1) / n))) * 180.0 / M_PI;
            double latCen = (latTop + latBot) / 2.0;
            CHECK(lat2tiley(latCen, z) == y_tms);
        }
    }
}

// Compute tile bounds and a tolerance of half a tile pixel for CHECK_NEAR calls.
struct TileBounds {
    double lonMin, lonMax, latLo, latHi, lonTol, latTol;
};
static TileBounds tileBounds(int zoom, int col, int row, int extent)
{
    double lonMin   = tilex2long(col,   zoom);
    double lonMax   = tilex2long(col+1, zoom);
    double latTileA = tiley2lat(row,    zoom);
    double latTileB = tiley2lat(row+1,  zoom);
    TileBounds b;
    b.lonMin = lonMin;
    b.lonMax = lonMax;
    b.latLo  = min(latTileA, latTileB);
    b.latHi  = max(latTileA, latTileB);
    b.lonTol = 0.5 * (lonMax - lonMin) / extent + 1e-9;
    b.latTol = 0.5 * (b.latHi - b.latLo) / extent + 1e-9;
    return b;
}

void test_vectortile_point_roundtrip()
{
    const int zoom = 10, col = 512, row = 512, extent = 4096;
    TileBounds b = tileBounds(zoom, col, row, extent);

    // Midpoint hits exact integer tile coords → no quantisation error
    double lonMid = b.lonMin + (b.lonMax - b.lonMin) * 0.5;
    double latMid = b.latLo  + (b.latHi  - b.latLo)  * 0.5;

    vector<Point2D>         pts   = {{lonMid, latMid}};
    vector<vector<Point2D>> lines;
    vector<Polygon2D>       polys;
    map<string, string>     tags  = {{"name", "test"}, {"pop", "1000"}};

    stringstream encoded;
    EncodeVectorTile enc(zoom, col, row, encoded);
    enc.LayerStart("places", 2, extent);
    enc.Feature((int)vector_tile::Tile_GeomType_POINT, false, 0, tags, pts, lines, polys);
    enc.LayerEnd();
    enc.Finish();

    CapturingResults results;
    DecodeVectorTile dec(zoom, col, row, results);
    dec.DecodeTileData(encoded.str());

    CHECK(results.layers.size() == 1);
    if (results.layers.empty()) return;
    CHECK(results.layers[0].name == "places");
    CHECK(results.layers[0].features.size() == 1);
    if (results.layers[0].features.empty()) return;
    const auto &f = results.layers[0].features[0];
    CHECK(f.type == (int)vector_tile::Tile_GeomType_POINT);
    CHECK(!f.hasId);
    CHECK(f.tags == tags);
    CHECK(f.points.size() == 1);
    if (f.points.empty()) return;
    CHECK_NEAR(f.points[0].first,  lonMid, b.lonTol);
    CHECK_NEAR(f.points[0].second, latMid, b.latTol);
}

void test_vectortile_linestring_roundtrip()
{
    const int zoom = 10, col = 512, row = 512, extent = 4096;
    TileBounds b = tileBounds(zoom, col, row, extent);

    // 0.25 / 0.75 fractions → exact integer tile coords → no quantisation error
    double lon1 = b.lonMin + (b.lonMax - b.lonMin) * 0.25;
    double lat1 = b.latLo  + (b.latHi  - b.latLo)  * 0.25;
    double lon2 = b.lonMin + (b.lonMax - b.lonMin) * 0.75;
    double lat2 = b.latLo  + (b.latHi  - b.latLo)  * 0.75;

    vector<Point2D>         pts;
    vector<vector<Point2D>> lines = {{{lon1, lat1}, {lon2, lat2}}};
    vector<Polygon2D>       polys;
    map<string, string>     tags  = {{"highway", "primary"}};

    stringstream encoded;
    EncodeVectorTile enc(zoom, col, row, encoded);
    enc.LayerStart("roads", 2, extent);
    enc.Feature((int)vector_tile::Tile_GeomType_LINESTRING, true, 42, tags, pts, lines, polys);
    enc.LayerEnd();
    enc.Finish();

    CapturingResults results;
    DecodeVectorTile dec(zoom, col, row, results);
    dec.DecodeTileData(encoded.str());

    CHECK(results.layers.size() == 1);
    if (results.layers.empty()) return;
    CHECK(results.layers[0].features.size() == 1);
    if (results.layers[0].features.empty()) return;
    const auto &f = results.layers[0].features[0];
    CHECK(f.type == (int)vector_tile::Tile_GeomType_LINESTRING);
    CHECK(f.hasId);
    CHECK(f.id == 42);
    CHECK(f.tags == tags);
    CHECK(f.lines.size() == 1);
    if (f.lines.empty()) return;
    CHECK(f.lines[0].size() == 2);
    if (f.lines[0].size() < 2) return;
    CHECK_NEAR(f.lines[0][0].first,  lon1, b.lonTol);
    CHECK_NEAR(f.lines[0][0].second, lat1, b.latTol);
    CHECK_NEAR(f.lines[0][1].first,  lon2, b.lonTol);
    CHECK_NEAR(f.lines[0][1].second, lat2, b.latTol);
}

void test_mbtilereader_nonexistent()
{
    // sqlite3_open creates a new empty database for unknown paths, then the
    // metadata query fails → constructor must throw.
    const char *path = "/tmp/test_mbtiles_no_metadata_xyz.db";
    remove(path);
    auto fn = [&]() { MBTileReader reader(path); };
    CHECK_THROWS(fn());
    remove(path);
}

void test_malformed_geometry_truncated()
{
    // MoveTo command with count=1 but no dx/dy values following
    vector_tile::Tile tile;
    auto *layer = tile.add_layers();
    layer->set_name("test"); layer->set_version(2); layer->set_extent(4096);
    auto *feature = layer->add_features();
    feature->set_type(vector_tile::Tile_GeomType_POINT);
    feature->add_geometry((1 & 0x7) | (1 << 3)); // MoveTo, count=1, no coords

    string tileData;
    tile.SerializeToString(&tileData);

    CapturingResults results;
    DecodeVectorTile dec(14, 9613, 9626, results);
    CHECK_THROWS(dec.DecodeTileData(tileData));
}

void test_malformed_tags_odd()
{
    // A feature with an odd number of tags (key without a paired value index)
    vector_tile::Tile tile;
    auto *layer = tile.add_layers();
    layer->set_name("test"); layer->set_version(2); layer->set_extent(4096);
    layer->add_keys("k");
    layer->add_values()->set_string_value("v");

    auto *feature = layer->add_features();
    feature->set_type(vector_tile::Tile_GeomType_POINT);
    feature->add_tags(0); // one tag only — odd
    feature->add_geometry((1 & 0x7) | (1 << 3)); // valid MoveTo, count=1
    feature->add_geometry(0);                     // dx=0
    feature->add_geometry(0);                     // dy=0

    string tileData;
    tile.SerializeToString(&tileData);

    CapturingResults results;
    DecodeVectorTile dec(14, 9613, 9626, results);
    CHECK_THROWS(dec.DecodeTileData(tileData));
}

// ---- Runner ----

#define RUN(fn) do { \
    cout << "  " << #fn << " ... " << flush; \
    int _before = failures; \
    fn(); \
    cout << (failures == _before ? "PASS" : "FAIL") << "\n"; \
} while(0)

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    RUN(test_gzip_roundtrip);
    RUN(test_gzip_empty);
    RUN(test_gzip_large);
    RUN(test_coord_conversion_roundtrip);
    RUN(test_vectortile_point_roundtrip);
    RUN(test_vectortile_linestring_roundtrip);
    RUN(test_mbtilereader_nonexistent);
    RUN(test_malformed_geometry_truncated);
    RUN(test_malformed_tags_odd);

    cout << "\n" << (total - failures) << "/" << total << " checks passed\n";
    google::protobuf::ShutdownProtobufLibrary();
    return failures > 0 ? 1 : 0;
}
