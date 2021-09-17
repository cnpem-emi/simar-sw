# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
