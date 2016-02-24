#ifndef FROG_SRC_CONFIGPARSER_H
#define FROG_SRC_CONFIGPARSER_H
#include <boost/property_tree/ptree.hpp>

namespace frog
{
class ConfigParser
{
 public:
  explicit ConfigParser(const std::string &conf_path_);
  ~ConfigParser() = default;
  
  std::string GetLogoPath();
  std::string GetDocumentRoot();
  unsigned short GetServerPort() const;
  int GetThreadsCount() const;
 private:
  const std::string conf_path_;
  boost::property_tree::ptree pt_;
};

} // namespace frog
#endif // FROG_SRC_CONFIGPARSER_H
