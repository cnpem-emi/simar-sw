# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.0] - 2021-11-25
### Added
- Monitors power factor, voltage glitches and frequency fully

### Changed
- Uses only one PRU for all PRU related measurements
- Wireless SIMAR now utilizes hidden SSIDs instead of a WPA-EAP connection
- Logs are concentrated in one file
- Startup has started being moved towards the bbb-function standard
- Merged BMP280/BME280 support into one library
- All required startup operations are done in one file; monitoring is done in separate services (to avoid reinits)

## [1.3.0] - 2021-10-14
### Added
- Wireless version can now work offline

### Changed
- Voltage is now measured once
- Better installation scripts

## [1.2.1] - 2021-10-14
### Added
- Preliminary support for power factor measurements

### Changed
- Check for unnatural peaks is more restringent
- Small compilation tweaks

## [1.2.0] - 2021-09-21
### Added
- Monitors voltage and current (up to 8 channels)
- Remote actuation functionalities
- Wireless (plug-and-play) version, capable of logging to external drives automatically
- BME280/BMP280 autodetection, allows different sensors on a single SIMAR
- More logging information

### Changed
- Fixes false positives when restoring pressure moving averages
- Reduces memory usage by freeing certain unused Redis reply objects (down to 1008 kB!)

## [1.1.0] - 2021-07-28
### Added
- Program now autodetects sensors; user only has to enter the desired names
- BMP280 functionality (fully on-par with BME280 functionalities)
- Automatically restarts the system if repeated failures are detected
- Program can now attempt to fetch external pressure data from multiple servers

### Changed
- Fixes "permanent lock" problem with door status algorithm caused by rapidly opening and closing the door
- Changed moving average window size from 10 to 5
- Increased polling rate
- Added open door pressure average to autorecovery (no more "bouncing" door status values)
- Moved to new Bosch driver for the BMP280
- Restored BMP280 code
- Altered sensor name limit from 6 to 16 characters
- Changed external pressure server address 

## [1.0.0] - 2021-07-22
### Added
- Determines door state by comparing it with internal rack pressure moving average
- Reads BME280 data
- Capable of reading/writing memory
- SPI/I2C helper functions
- Updated SPIxxCON lib
- Other sensors and communication methods
- Rapid GPIO/I2C bus management using direct register access
