# Blockchain-Integrated Intelligent Multi-User Smart Meter

This repository contains the embedded systems firmware for a **Blockchain-Integrated Intelligent Multi-User Metering System** designed for real-time electrical energy monitoring, AI-assisted electricity theft detection, and efficient energy management.

The system integrates ESP32 microcontrollers, PZEM-004T energy monitoring modules, edge computing, artificial intelligence (AI), IoT connectivity, and blockchain technology to monitor energy consumption, identify suspicious consumption patterns, and improve the integrity and traceability of energy records.

The prototype is designed for multi-user electricity monitoring within a building, such as a hostel or lodge.

## Key Features

- **Real-Time Energy Monitoring:** Measures electrical parameters and energy consumption using PZEM-004T energy monitoring modules.
- **ESP32-Based Embedded System:** Acquires and processes meter readings directly on microcontrollers.
- **AI-Based Electricity Theft Detection:** Applies machine learning to identify abnormal energy consumption patterns that may indicate electricity theft or meter tampering.
- **Edge Computing:** Supports local processing of energy data and theft-detection logic close to the metering hardware, reducing dependence on continuous cloud communication.
- **Multi-User Energy Metering:** Monitors individual users' energy consumption within a shared building.
- **IoT Connectivity:** Transmits meter readings and relevant detection results to connected cloud services.
- **Cloud Integration:** Uses Firebase for storing and retrieving energy readings and supporting remote monitoring.
- **Blockchain Integration:** Uses Solana to record and verify selected energy consumption records, supporting data traceability and tamper evidence.
- **Energy Analysis:** Provides data for consumption analysis, anomaly identification, and energy management.

## System Architecture

The system combines embedded hardware, edge intelligence, cloud services, and blockchain technology.

1. **Energy Measurement:** PZEM-004T modules measure electrical parameters and energy consumption.
2. **Embedded Data Acquisition:** ESP32 microcontrollers collect meter readings and process the data.
3. **Edge-Based Theft Detection:** Local detection logic and, where deployed, machine learning models analyse consumption patterns for suspicious behaviour.
4. **IoT Data Transmission:** Meter readings and detection results are transmitted to connected services.
5. **Cloud Monitoring:** Firebase stores and provides access to meter data for monitoring and analysis.
6. **Blockchain Record Integrity:** Selected energy records are submitted to Solana for verifiable, tamper-evident recordkeeping.

## Technologies

- **Microcontroller:** ESP32
- **Energy Monitoring:** PZEM-004T
- **Firmware Development:** C++ / Arduino IDE
- **Edge Computing:** Local energy-data processing and theft-detection logic
- **Machine Learning:** AI-assisted energy theft and consumption anomaly detection
- **Communication:** Wi-Fi and serial communication
- **Cloud Backend:** Firebase
- **Blockchain:** Solana
- **Data Analysis and Model Training:** Python and relevant machine learning libraries

## AI-Based Electricity Theft Detection

A key research component of this project is the application of machine learning to electricity theft detection.

The detection approach analyses energy consumption measurements and identifies deviations from expected consumption patterns. Depending on the implemented model and available features, suspicious behaviour may include unexplained reductions in measured consumption, inconsistencies between upstream and individual-user meter readings, and other abnormal usage patterns.

The edge-computing approach aims to enable energy data to be analysed near the source, supporting timely detection without requiring every measurement to be sent to a remote server for inference.

**Important:** An AI-generated anomaly is an indication for further investigation, not conclusive proof of electricity theft. Actual detection performance depends on the training dataset, model, sensor accuracy, and deployment environment.

## Project Objectives

- Develop an ESP32-based embedded smart metering system.
- Monitor electricity consumption for multiple users within a building.
- Implement edge-based processing for efficient energy monitoring.
- Develop AI-assisted methods for identifying suspicious energy consumption patterns.
- Integrate IoT connectivity for remote monitoring and data management.
- Integrate blockchain technology to improve the traceability and integrity of selected meter records.
- Support more transparent and efficient energy management.

## Intended Application

The prototype is intended for building-level electricity monitoring, particularly in hostels, lodges, and other multi-user residential environments where individual energy consumption needs to be measured and monitored.

## Repository Contents

The repository may contain the following components as they are implemented:

- ESP32 firmware and sensor interfaces.
- PZEM-004T energy measurement code.
- Edge-processing and theft-detection logic.
- Machine learning model training and evaluation scripts.
- Firebase connectivity and data-management code.
- Solana blockchain integration.
- Hardware configuration and wiring documentation.
- Setup instructions and testing procedures.

## Project Status

This project is a prototype integrating embedded systems, IoT, edge computing, AI-assisted electricity theft detection, and blockchain-backed energy recordkeeping. Feature availability depends on the specific firmware, trained model, hardware configuration, and backend deployment.

## Research Focus

**Embedded Systems | Smart Metering | Internet of Things (IoT) | Edge AI | Electricity Theft Detection | Blockchain | Energy Management**
