add_rules("mode.debug", "mode.release")

includes("./src/module/setting")
includes("./src/module/log")
includes("./src/module/common")
includes("./src/module/protocol")

add_requires("jsoncpp")
add_includedirs("./src/")

target("StarGui")
    add_packages("jsoncpp")
    add_deps("Logger")
    add_rules("qt.widgetapp")
    add_headerfiles("src/*.h")
    add_files("src/core/*.cpp")
    add_files("src/core/mainwindow.ui")
    -- add files with Q_OBJECT meta (only for qt.moc)
    add_files("src/core/mainwindow.h")
    add_frameworks("QtNetwork")
target_end()