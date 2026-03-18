import SwiftUI

struct ConnectView: View {
    @EnvironmentObject var connection: ConnectionManager
    @State private var editingIP = ""
    @State private var showModeHelp = false

    var body: some View {
        NavigationView {
            Form {
                // Status
                Section {
                    HStack(spacing: 12) {
                        Circle()
                            .fill(connection.isConnected ? Color.green : Color.red)
                            .frame(width: 14, height: 14)
                        Text(connection.isConnected ? "Connected" : "Disconnected")
                            .font(.headline)
                        Spacer()
                        if connection.isConnected {
                            Text(connection.lastStatus.ip.isEmpty ? connection.ipAddress : connection.lastStatus.ip)
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }
                    }
                    .padding(.vertical, 4)
                } header: { Text("Status") }

                // IP Entry
                Section {
                    HStack {
                        Text("ESP IP Address")
                        Spacer()
                        TextField("192.168.4.1", text: $connection.ipAddress)
                            .keyboardType(.decimalPad)
                            .multilineTextAlignment(.trailing)
                            .foregroundColor(.orange)
                    }
                    HStack {
                        Text("Port")
                        Spacer()
                        Text("81")
                            .foregroundColor(.secondary)
                    }
                } header: { Text("Network") }
                footer: {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("AP mode (ESP hotspot): 192.168.4.1")
                        Text("STA mode (your router): check Serial Monitor")
                        Text("iPhone hotspot: check DHCP leases")
                    }
                    .font(.caption)
                }

                // Connect button
                Section {
                    if connection.isConnected {
                        Button(role: .destructive) {
                            connection.disconnect()
                        } label: {
                            HStack {
                                Spacer()
                                Text("Disconnect")
                                    .fontWeight(.semibold)
                                Spacer()
                            }
                        }
                    } else {
                        Button {
                            connection.connect()
                        } label: {
                            HStack {
                                Spacer()
                                Text("Connect")
                                    .fontWeight(.semibold)
                                    .foregroundColor(.white)
                                Spacer()
                            }
                            .padding(.vertical, 4)
                        }
                        .listRowBackground(Color.orange)
                    }
                }

                // Error
                if let error = connection.errorMessage {
                    Section {
                        Text(error)
                            .foregroundColor(.red)
                            .font(.caption)
                    } header: { Text("Error") }
                }

                // Live status
                if connection.isConnected {
                    Section {
                        StatusRow(label: "Direction", value: directionLabel)
                        StatusRow(label: "Speed",     value: "\(Int(Double(connection.lastStatus.speed)/255*100))%")
                        StatusRow(label: "Limit A",   value: connection.lastStatus.limitA ? "TRIGGERED" : "Clear",
                                  valueColor: connection.lastStatus.limitA ? .red : .green)
                        StatusRow(label: "Limit B",   value: connection.lastStatus.limitB ? "TRIGGERED" : "Clear",
                                  valueColor: connection.lastStatus.limitB ? .red : .green)
                    } header: { Text("Live Carriage Status") }
                }

                // Mode help
                Section {
                    DisclosureGroup("WiFi Mode Guide", isExpanded: $showModeHelp) {
                        VStack(alignment: .leading, spacing: 10) {
                            ModeHelpRow(
                                title: "AP Mode (Hotspot)",
                                body: "Set config.h WIFI_MODE to WIFI_MODE_AP. ESP creates \"FlagCarriage\" network. Connect iPhone to that network. IP is always 192.168.4.1."
                            )
                            ModeHelpRow(
                                title: "STA Mode (Router)",
                                body: "Set config.h WIFI_MODE to WIFI_MODE_STA with your credentials. Open Serial Monitor at 115200 baud — ESP prints its IP on startup."
                            )
                            ModeHelpRow(
                                title: "iPhone Hotspot",
                                body: "Set STA mode with your iPhone hotspot name/password. In Settings > Personal Hotspot, check connected devices to find the IP."
                            )
                        }
                        .padding(.top, 4)
                    }
                }
            }
            .navigationTitle("Connect")
        }
    }

    var directionLabel: String {
        switch connection.lastStatus.direction {
        case "F": return "Forward"
        case "B": return "Backward"
        default:  return "Stopped"
        }
    }
}

struct StatusRow: View {
    let label: String
    let value: String
    var valueColor: Color = .primary
    var body: some View {
        HStack {
            Text(label).foregroundColor(.secondary)
            Spacer()
            Text(value).foregroundColor(valueColor).fontWeight(.medium)
        }
    }
}

struct ModeHelpRow: View {
    let title: String
    let body: String
    var body_: some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(title).fontWeight(.semibold).font(.subheadline)
            Text(body).font(.caption).foregroundColor(.secondary)
        }
    }
    var body: some View { body_ }
}
