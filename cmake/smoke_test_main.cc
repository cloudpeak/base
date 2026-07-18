// Smoke test main for the CMake-built libbase.
// Just exercises a couple of base APIs to confirm the library links.

#include "base/at_exit.h"
#include "base/base64.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_util.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);

  const std::string input = "hello cmake base";
  std::string encoded = base::Base64Encode(input);
  std::string decoded;
  base::Base64Decode(encoded, &decoded);

  LOG(INFO) << "base64 round trip: " << decoded;
  std::cout << "encoded = " << encoded << "\n";
  std::cout << "decoded = " << decoded << "\n";
  return decoded == input ? 0 : 1;
}
