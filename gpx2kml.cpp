/**
 * Convert a GPX file from mytracks to a google earth KML file.
 * You can have mytracks output a KML file directly, but it's
 * actually not a very good one. It puts the points in the GPX
 * namespace. Google Earth pretends it can render them, but it
 * totally messes up the altitude. Since the altitude is the
 * main thing I'm interested in, that won't do at all.
 *
 * Copyright 2013 Bruce Ide
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include "coordinates.hpp"
#include <fstream>
#include "gpx_factory.hpp"
#include <iostream>
#include "kml_document.hpp"
#include "kml_folder.hpp"
#include "kml_placemark.hpp"
#include "kml_point.hpp"
#include "kml_linestring.hpp"
#include <memory>
#include "string_converts.hpp"
#include "timezone_manager.hpp"
#include <utility>
#include <vector>
#include "xml_node.hpp"

// gpx factory notifies with the posix time as a double and a lat_long
// coordinate (But I want my coordinates in ECEF for later analysis)
typedef std::pair<double, fr::coordinates::ecef> coordinate_pair;
typedef std::vector<coordinate_pair> coordinate_vector;

// Populate_coordinates is the listener for the gpx factory
void populate_coordinates(coordinate_vector *coordinates, double at_time, fr::coordinates::lat_long point)
{
  fr::coordinates::ecef point_ecef = fr::coordinates::converter<fr::coordinates::ecef>()(point);
  coordinates->push_back(std::make_pair(at_time, point_ecef));
}

// Print KML document to selected stream
void output_kml(std::ostream &out, cppxml::xml_node::pointer kml_document)
{
  std::string xml_header("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  out << xml_header << std::endl << kml_document->to_string();
}

// Add a coordinate for a point. I include last point in case you
// want to compute deltas or something
void add_coordinate(cppxml::kml_linestring::pointer to_this, const coordinate_pair &coordinates, double last_time, fr::coordinates::ecef last_point)
{
  timeval tv;
  
  // Dropping factions for the time being. 
  tv.tv_usec = 0;
  tv.tv_sec = (long) coordinates.first;

  to_this->add(coordinates.second);
  
}

int main(int argc, char *argv[])
{
  std::string input_filename;
  // Affix timezone to GMT (Which is what the GPX file format is in)
  fr::time::timezone_manager timezone_manager("GMT");
  boost::program_options::options_description desc("Usage");
  desc.add_options()
    ("help,h", "Print help")
    ("input_filename,i",
     boost::program_options::value<std::string>(&input_filename)->required(),
     "GPX file from mytracks");
  boost::program_options::variables_map vm;
  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  } catch (std::exception e) {
    std::cout << "Caught an exception while parsing command line options" << std::endl;
    std::cout << e.what() << std::endl;
    std::cout << std::endl << "Try -h for help" << std::endl;
    exit(1);
  } catch (...) {
    std::cout << "Caught an unknown exception while parsing command line options" << std::endl;
    std::cout << "Try -h for help" << std::endl;
    exit(1);
  }

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    exit(1);
  }

  try {
    boost::program_options::notify(vm);
  } catch (std::exception e) {
    std::cout << e.what() << std::endl;
    exit(1);
  } catch (...) {
    std::cout << "Caught exception while handling command line arguments" << std::endl;
    std::cout << "Try -h for help" << std::endl;
    exit(1);
  }

  coordinate_vector coordinates;
  fr::data::gpx_factory factory(input_filename);
  factory.available.connect(boost::bind(&populate_coordinates, &coordinates, _1, _2));
  factory.process();
  // We should now have a vector of coordinates all ready to grind up into
  // a yummy coordinate sausage!

  // Lets just get it out as quickly as possible right now
  double last_time = 0.0;
  fr::coordinates::ecef last_point(0.0, 0.0, 0.0);

  cppxml::kml_document::pointer document = std::make_shared<cppxml::kml_document>("gpx2kml output");
  cppxml::kml_folder::pointer folder = std::make_shared<cppxml::kml_folder>("Coordinates");
  cppxml::kml_placemark::pointer placemark = std::make_shared<cppxml::kml_placemark>();
  cppxml::kml_linestring::pointer linestring = std::make_shared<cppxml::kml_linestring>();

  folder->add_child(placemark);
  placemark->add_child(linestring);
  document->add_child(folder);

  for (const coordinate_pair &pair : coordinates) {
    add_coordinate(linestring, pair, last_time, last_point);
    last_time = pair.first;
    last_point = pair.second;
  }
  
  output_kml(std::cout, document->to_xml());

}
