#include "ConfigParser.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace frog
{
namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

ConfigParser::ConfigParser(const std::string &conf_path)
  : conf_path_(conf_path)
{
  pt::read_xml(conf_path_, pt_);
}

std::string ConfigParser::GetLogoPath() {
  std::string logo_path = pt_.get<std::string>("frog.LogoLocate");
  // check it is relative path or absolute path.
  fs::path lp(logo_path);
  if (lp.is_absolute()) {
    return logo_path;
  } else {
    return system_complete(lp).string();
  }
}

std::string ConfigParser::GetDocumentRoot() {
  std::string doc_root = pt_.get<std::string>("frog.DocumentRoot");
  fs::path dp(doc_root);
  if (dp.is_absolute()) {
    return doc_root;
  } else {
    return system_complete(dp).string();
  }
}

unsigned short ConfigParser::GetServerPort() const {
  return pt_.get<unsigned short>("frog.ServerPort");
}

int ConfigParser::GetThreadsCount() const {
  return pt_.get<int>("frog.ThreadsCount");
}

} // namespace frog
