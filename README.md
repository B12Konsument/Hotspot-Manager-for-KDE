# Hotspot Manager for KDE

Eine kleine KDE/Plasma-native Qt6-Anwendung zum Erstellen und Verwalten eines
NetworkManager-WLAN-Hotspots unter Arch Linux.

## Funktionen

- WLAN-Geräte aus NetworkManager anzeigen
- Hotspot-SSID, Passwort, Sicherheitsmodus, Band und Kanal setzen
- IPv4-Internetfreigabe über NetworkManager aktivieren
- Hotspot starten, stoppen und Status aktualisieren
- Bestehende Hotspot-Verbindung wiederverwenden
- Optionalen Autostart der NetworkManager-Verbindung setzen

## Abhängigkeiten unter Arch Linux

```bash
sudo pacman -S base-devel cmake qt6-base networkmanager
sudo systemctl enable --now NetworkManager
```

`nmcli` muss verfügbar sein. Es ist Teil des Pakets `networkmanager`.

## Bauen

```bash
cmake -S . -B build
cmake --build build
./build/hotspot-manager-kde
```

## Installieren

```bash
cmake --install build
```

Je nach System kann dafür `sudo` nötig sein.

## Hinweise

Der Hotspot nutzt NetworkManagers `ipv4.method shared`. Das entspricht dem
üblichen Linux-Weg für eine Windows-ähnliche mobile Hotspot-Funktion: Clients
bekommen per DHCP eine Adresse, und der Host teilt seine Internetverbindung per
NAT.
