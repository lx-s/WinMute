# Datenschutzerklärung für WinMute

*English version: [PRIVACY.md](PRIVACY.md)*

**Gilt für:** WinMute für Windows, alle Versionen und Vertriebswege
**Stand:** 25.08.2026
**Verantwortlicher:** Alexander Steinhoefer (lx-systems)
**Kontakt:** kontakt@lx-s.de

## Kurzfassung

WinMute erhebt, überträgt, verkauft und teilt keine personenbezogenen Daten.

Es gibt keine Benutzerkonten, keine Werbung, keine Analyse- und keine
Telemetriefunktionen. Alles, was WinMute für seine Arbeit benötigt, verbleibt
auf Ihrem Gerät. Die einzige optionale Netzwerkfunktion ist die Suche nach neuen
Programmversionen – und die ist ausgeschaltet, solange Sie sie nicht selbst
einschalten (siehe Abschnitt 5).

## 1. Was WinMute macht

WinMute ist ein kleines Hilfsprogramm, das die Audiogeräte Ihres Computers
automatisch stummschaltet und die Stummschaltung wieder aufhebt – zum Beispiel
wenn Sie Ihren Arbeitsplatz sperren, wenn sich der Bildschirm abschaltet, wenn
die Verbindung zu Ihrem Bluetooth-Kopfhörer getrennt wird oder wenn Sie sich mit
einem bestimmten WLAN-Netzwerk verbinden.

All das geschieht lokal auf Ihrem Rechner über die dafür vorgesehenen
Windows-Schnittstellen.

## 2. Daten, die WinMute auf Ihrem Gerät speichert

WinMute speichert Ihre Einstellungen, damit sie einen Neustart überdauern. Diese
Daten verlassen Ihren Computer nicht, werden nirgendwohin hochgeladen und sind
ausschließlich unter Ihrem eigenen Windows-Benutzerkonto lesbar.

Zu den Einstellungen gehören:

* Welche Auslöser für das Stummschalten Sie aktiviert haben (Sperren des
  Arbeitsplatzes, Bildschirmabschaltung, Energiesparmodus, Herunterfahren,
  Abmelden, Zuklappen des Notebooks, Remotedesktop und so weiter).
* Die Namen der WLAN-Netzwerke (SSIDs), die Sie auf Ihre Stummschalt- bzw.
  Erlaubt-Liste gesetzt haben.
* Die Namen und Adressen der Bluetooth-Geräte, die Sie in Ihre Geräteliste
  aufgenommen haben.
* Die Namen der Audiogeräte, die Sie einzeln verwalten lassen.
* Ihre Ruhezeiten.
* Oberflächeneinstellungen wie die Anzeigesprache, die Benachrichtigungsoptionen
  und Ihr globaler Hotkey zum Stummschalten.

Die Einstellungen werden in der Windows-Registrierung unter Ihrem eigenen
Benutzerkonto abgelegt, im Schlüssel
`HKEY_CURRENT_USER\SOFTWARE\lx-systems\WinMute`. Haben Sie WinMute aus dem
Microsoft Store bezogen, speichert Windows diese Daten stattdessen im
app-eigenen, benutzerbezogenen Speicher des Pakets.

## 3. Informationen, die WinMute aus Ihrem System ausliest

Um zu entscheiden, wann stummgeschaltet werden soll, liest WinMute zur Laufzeit
die folgenden Informationen aus Windows aus. Sie werden im Arbeitsspeicher
ausgewertet, für die Entscheidung genutzt und anschließend verworfen. Über das
hinaus, was Sie nach Abschnitt 2 selbst konfiguriert haben, werden sie nicht
gespeichert und niemals übertragen.

* **Audiogeräte:** die Namen sowie den Stummschalt- und Lautstärkezustand Ihrer
  Wiedergabegeräte, um sie stummschalten und wiederherstellen zu können.
* **WLAN:** den Namen (SSID) des WLAN-Netzwerks, mit dem Sie gerade verbunden
  sind, um ihn mit Ihrer eigenen Liste abzugleichen. WinMute liest weder Ihre
  WLAN-Kennwörter noch gespeicherte Profile oder Ihren Netzwerkverkehr.
* **Bluetooth:** den Zustand Ihres Bluetooth-Funkmoduls sowie die Namen von
  Geräten, die sich verbinden oder trennen, um auf Ihre Kopfhörer reagieren zu
  können. WinMute koppelt keine Geräte und greift nicht auf deren Inhalte zu.
* **Sitzungs- und Energieereignisse:** Benachrichtigungen von Windows darüber,
  dass der Arbeitsplatz gesperrt oder der Bildschirm abgeschaltet wurde oder das
  System in den Energiesparmodus wechselt.
* **Medienwiedergabe:** Wenn Sie die Option aktivieren, sendet WinMute einen
  Pause- oder Wiedergabebefehl an Ihre Medienanwendungen. Was Sie abspielen,
  liest WinMute nicht aus.

## 4. Optionale Protokolldatei

Die Protokollierung ist **standardmäßig ausgeschaltet**. Schalten Sie sie in den
Einstellungen ein – etwa weil Sie im Rahmen einer Fehlermeldung darum gebeten
wurden –, schreibt WinMute eine Protokolldatei im Klartext in Ihren
Windows-Ordner für temporäre Dateien (`%TEMP%`).

Das Protokoll hält fest, was WinMute getan hat und warum; darin können die Namen
Ihrer Audiogeräte, WLAN-Netzwerke und Bluetooth-Geräte auftauchen. Die Datei
bleibt auf Ihrem Computer, WinMute lädt sie niemals hoch.

Schalten Sie die Protokollierung wieder aus, löscht WinMute die Protokolldatei.
Sie können sie auch jederzeit selbst löschen.

Wenn Sie das Protokoll einer öffentlichen Fehlermeldung beifügen möchten, sehen
Sie es sich bitte vorher an – was Sie uns senden, geben Sie aus eigenem
Entschluss preis, nicht das Programm.

## 5. Netzwerkverbindungen und die Updateprüfung

Der einzige Teil von WinMute, der das Internet nutzt, ist die optionale
Updateprüfung, und sie ist **standardmäßig ausgeschaltet**. Solange Sie in den
Einstellungen nicht „Bei Programmstart nach Updates suchen“ ankreuzen, baut
WinMute überhaupt keine Verbindung auf.

Aktivieren Sie sie, lädt WinMute bei jedem Programmstart eine einzige kleine,
statische Textdatei über eine verschlüsselte HTTPS-Verbindung herunter:

```
https://raw.githubusercontent.com/lx-s/WinMute/main/CURRENT_VERSION
```

Diese Datei enthält die Nummer der aktuellsten verfügbaren Version. WinMute
vergleicht sie mit der bei Ihnen laufenden Version und bietet Ihnen, falls Ihre
älter ist, einen Link zur Downloadseite an. Es wird nichts hochgeladen: Die
Anfrage enthält keinerlei Angaben über Sie, Ihr Gerät, Ihre Einstellungen oder
Ihre Nutzung – nicht einmal die bei Ihnen installierte Version.

Wie bei jedem Download sieht der Server, der die Datei bereitstellt (GitHub,
betrieben von GitHub, Inc.), zwangsläufig Ihre IP-Adresse und den Zeitpunkt der
Anfrage, um sie überhaupt beantworten zu können. Diese Daten werden nach der
[Datenschutzerklärung von GitHub](https://docs.github.com/de/site-policy/privacy-policies/github-general-privacy-statement)
verarbeitet. Wir erhalten darüber keinerlei Rückmeldung und können nicht
nachvollziehen, wer nach Updates gesucht hat. Wenn Sie GitHub gar nicht erst
kontaktieren möchten, lassen Sie die Updateprüfung einfach ausgeschaltet.

In den über den Microsoft Store vertriebenen Fassungen ist die Updateprüfung
vollständig deaktiviert und lässt sich auch nicht einschalten, da der Store das
Programm selbst aktuell hält.

Darüber hinaus enthält WinMute keinerlei Netzwerkfunktionen.

## 6. Woher Sie WinMute bezogen haben

Diese Erklärung betrifft die Anwendung WinMute selbst. Wo Sie sie bezogen haben,
ist davon zu trennen: Herunterladen, Installieren und Aktualisieren über einen
Store oder Paketmanager bindet den jeweiligen Anbieter ein, und zwar nach dessen
eigener Datenschutzerklärung und nicht nach unserer. Je nach Bezugsweg ist das
der [Microsoft Store](https://privacy.microsoft.com/de-de/privacystatement),
[winget](https://privacy.microsoft.com/de-de/privacystatement),
[Chocolatey](https://chocolatey.org/privacy) oder – bei direkten Downloads und
Releases –
[GitHub](https://docs.github.com/de/site-policy/privacy-policies/github-general-privacy-statement).

Von diesen Anbietern erhalten wir allenfalls anonyme, zusammengefasste
Kennzahlen wie Download- oder Installationszahlen. Diese lassen keinen Rückschluss
auf einzelne Personen zu, und wir können sie weder einer Person noch einem Gerät
zuordnen.

Sollte WinMute abstürzen, kann die Windows-Fehlerberichterstattung entsprechend
Ihren Windows-Einstellungen Diagnosedaten an Microsoft senden. Das ist eine
Funktion von Windows und liegt außerhalb unseres Einflussbereichs.

## 7. Weitergabe an Dritte

Wir erheben keine personenbezogenen Daten, also gibt es auch nichts
weiterzugeben. Wir verkaufen, vermieten und übermitteln keine Daten an Dritte.
WinMute enthält keine Werbenetzwerke, keine Analyse-SDKs und keine
Tracking-Komponenten Dritter.

## 8. Speicherdauer und Löschung

Ihre Einstellungen bleiben so lange auf Ihrem Gerät, wie Sie sie dort belassen.

* **Microsoft Store:** Beim Deinstallieren über Windows wird die App zusammen mit
  den gespeicherten Einstellungen entfernt.
* **Setup oder Paketmanager:** Deinstallieren Sie WinMute wie gewohnt. Ihre
  persönlichen Einstellungen bleiben dabei bewusst erhalten, damit sie eine
  Neuinstallation überdauern. Um auch diese zu entfernen, löschen Sie den
  Schlüssel `HKEY_CURRENT_USER\SOFTWARE\lx-systems\WinMute` mit dem
  Windows-Registrierungs-Editor (`regedit.exe`).
* **Portable Fassung (ZIP):** Löschen Sie den WinMute-Ordner – und, wenn auch die
  Einstellungen verschwinden sollen, zusätzlich den oben genannten
  Registrierungsschlüssel.

Die optionale Protokolldatei löschen Sie, indem Sie entweder die Protokollierung
in WinMute ausschalten – dann wird sie sofort gelöscht – oder die
WinMute-Protokolldatei aus Ihrem `%TEMP%`-Ordner entfernen.

## 9. Kinder

WinMute ist ein Hilfsprogramm für den allgemeinen Gebrauch und richtet sich nicht
an Kinder. Es erhebt wissentlich von niemandem Daten, unabhängig vom Alter.

## 10. Ihre Rechte (DSGVO)

Wir verarbeiten keine personenbezogenen Daten im Sinne der DSGVO. Wir betreiben
weder Server noch einen Dienst, mit dem WinMute kommuniziert: Wir haben keinen
Zugriff auf Ihre Einstellungen, Ihr Gerät oder irgendeine Kennung. Selbst wenn
Sie die optionale Updateprüfung einschalten, geht Ihre Anfrage an GitHub als
Anbieter der Datei und erreicht uns zu keinem Zeitpunkt.

Bei uns liegen daher keine Daten vor, über die wir Auskunft erteilen könnten
(Art. 15 DSGVO) und die wir berichtigen (Art. 16 DSGVO), löschen (Art. 17
DSGVO), in ihrer Verarbeitung einschränken (Art. 18 DSGVO) oder übertragen
(Art. 20 DSGVO) könnten oder gegen deren Verarbeitung Sie Widerspruch einlegen
könnten (Art. 21 DSGVO). Unabhängig davon steht Ihnen das Recht zu, sich bei
einer Datenschutz-Aufsichtsbehörde zu beschweren (Art. 77 DSGVO).

Über die in den Abschnitten 2 und 4 beschriebenen Daten behalten Sie jederzeit
die volle Kontrolle, da sie ausschließlich auf Ihrem eigenen Computer existieren.

Sollten Sie das anders sehen oder Fragen dazu haben, wie WinMute mit Daten
umgeht, wenden Sie sich gerne an die unten genannte Adresse.

## 11. Quelloffenheit

WinMute ist quelloffen und steht unter der 3-Klausel-BSD-Lizenz. Jede Aussage
dieser Erklärung lässt sich im Quelltext nachprüfen:
<https://github.com/lx-s/WinMute>.

## 12. Änderungen dieser Datenschutzerklärung

Ändert sich diese Datenschutzerklärung, wird die aktualisierte Fassung an dieser
Stelle veröffentlicht und das Datum am Anfang entsprechend angepasst.
Wesentliche Änderungen werden zusätzlich im Änderungsprotokoll des Programms
vermerkt.

## 13. Kontakt

Alexander Steinhoefer (lx-systems)
E-Mail: kontakt@lx-s.de
Projektseite: <https://github.com/lx-s/WinMute>
