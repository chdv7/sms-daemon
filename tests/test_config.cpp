#include <gtest/gtest.h>

#include <fstream>

#include "../src/Config.h"

namespace chdv::sms_daemon::test {

TEST(ConfigTest, LoadsDevice) {
    const std::string path = "/tmp/sms-daemon-config-test.conf";
    std::ofstream(path) << "# modem port\n device = /dev/ttyUSB2 \n";

    SmsDaemonConfig config;
    std::string error;
    ASSERT_TRUE(LoadSmsDaemonConfig(path, config, error)) << error;
    EXPECT_EQ(config.device, "/dev/ttyUSB2");
}

TEST(ConfigTest, RejectsUnknownSetting) {
    const std::string path = "/tmp/sms-daemon-bad-config.conf";
    std::ofstream(path) << "unknown=value\n";

    SmsDaemonConfig config;
    std::string error;
    EXPECT_FALSE(LoadSmsDaemonConfig(path, config, error));
    EXPECT_NE(error.find("unknown setting"), std::string::npos);
}

} // namespace chdv::sms_daemon::test
