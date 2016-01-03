#! /usr/bin/ruby

Dir.chdir(File.dirname($0))

require 'Tempfile'
BUILD_LOG = Tempfile.new("DrudgeNet-")
BUILD_LOG_PATH = BUILD_LOG.path

def log(string)
	puts string
	open(BUILD_LOG_PATH, 'a'){|f| f.puts string}
end

PROJECT = "DrudgeNet.xcodeproj"
VERBOSE = (not ARGV.include?("--quiet"))

def system(command)
	log "> #{command}"

	result = Kernel.system(VERBOSE ? "#{command} | tee -a #{BUILD_LOG_PATH}; exit ${PIPESTATUS[0]}" : "#{command} >> #{BUILD_LOG_PATH}")
	unless $? == 0
		log "==========================================="
		log "Command failed with status #{$?}: #{command}"
		log "Build errors encountered. Aborting build script"
		log "Check the build log for more information: #{BUILD_LOG_PATH}"
		raise
	end
end

def build(target, configuration)
	command = "xcodebuild -project #{PROJECT} -configuration #{configuration} -arch x86_64 -target #{target}"
	system command

	return "build/#{configuration}/lib#{target}.a"
end

def build_lib(target, copy_list)
	debug_lib = build(target, "Debug")
	release_lib = build(target, "Release")

	dirname = "lib#{target}"

	system "rm -rf '#{dirname}'"
	system "mkdir '#{dirname}'"

	system "cp '#{debug_lib}' '#{dirname}/lib#{target}-Debug.a'"
	system "cp '#{release_lib}' '#{dirname}/lib#{target}.a'"

	system "mkdir '#{dirname}'/include"

	system "cp ./DrudgeNet/include/*.h '#{dirname}/include'"

	system "rm -rf build"
end

build_lib("DrudgeNet", [
	"include",
])

BUILD_LOG.delete
