/*
 * Copyright 2005-2014, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		René Gollent
 *		Wim van der Meer <WPJvanderMeer@gmail.com>
 */
#ifndef CREDITS_H
#define CREDITS_H


typedef struct {
	const char* languageCode;
	const char* names;
} Translation;


static const Translation kTranslations[] = {
#if 0
	{ "ar",
		"Khaled Berraoui (khallebal)\n"
		"Kendhia\n"
		"softity\n"
	},
#endif
	{ "be",
		"Aliya\n"
		"Michael Bulash\n"
		"Siaržuk Žarski\n"
	},
	{ "bg",
		"Ognyan Valeri Angelov\n"
		"cssvb94\n"
		"naydef\n"
	},
	{ "cs",
		"Pavel Drotár\n"
		"Martin Janiczek\n"
		"Matěj Kocián\n"
		"zafan\n"
	},
	{ "da",
		"Brian Matzon\n"
		"Kristian Poul Herkild\n"
	},
	{ "de",
		"Atalanttore\n"
		"Colin Günther\n"
		"Mirko Israel\n"
		"leszek\n"
		"Christian Morgenroth\n"
		"Aleksas Pantechovskis\n"
		"Niels Sascha Reedijk\n"
		"Joachim Seemer (Humdinger)\n"
		"Matthias Spreiter\n"
		"Ivaylo Tsenkov\n"
		"svend\n"
		"taos\n"
	},
	{ "el",
		"Γιάννης Κωνσταντινίδης [Giannis Konstantinidis] (giannisk)\n"
		"Βαγγέλης Μαμαλάκης [Vaggelis Mamalakis]\n"
		"Άλεξ-Π. Νάτσιος [Alex-P. Natsios] (Drakevr)\n"
		"JamesSP472\n"
		"vasper\n"
	},
	{ "en_ca",
		"Edwin Amsler\n"
		"infamy\n"
	},
	{ "en_gb",
		"Harsh Vardhan\n"
		"Jessica_Lily\n"
		"Richie Nyhus\n"
	},
	{ "eo",
		"Travis D. Reed (Dancxjo)\n"
	},
	{ "es",
		"Pedro Arregui\n"
		"Zola Bridges\n"
		"Nicolás C (CapitanPico)\n"
		"Oscar Carballal (oscarcp)\n"
		"Luis Gustavo Lira\n"
		"Louis de M.\n"
		"Victor Madariaga\n"
		"César Ortiz Pantoja (ccortiz)\n"
		"Miguel Zúñiga González (miguel~1.mx)\n"
		"c-sanchez\n"
		"EdiNOS\n"
		"espectalll123\n"
		"fatigatti\n"
		"mpescador\n"
		"OscarL\n"
		// "taos\n" bugfixes only, no actual translations
	},
	{ "fi",
		"Jorma Karvonen (Karvjorm)\n"
		"Jaakko Leikas (Garjala)\n"
		"Niels Sascha Reedijk\n"
		"Slavi Stefanov Sotirov\n"
		"nortti\n"
	},
	{ "fr",
		"Edwin Amsler\n"
		"Yannick Barbel\n"
		"Jean-Loïc Charroud\n"
		"Adrien Destugues (PulkoMandy)\n"
		"Niels Sascha Reedijk\n"
		"Florent Revest\n"
		"Harsh Vardhan\n"
		"Adeimantos\n"
		"Blazkowitz\n"
		"Loïc\n"
		"roptat\n"
		"Wabouz\n"
	},
	{ "hi",
		"Abhishek Arora\n"
		"Dhruwat Bhagat\n"
		"Jayneil Dalal\n"
		"Atharva Lath\n"
		"Harsh Vardhan\n"
		"unitedroad\n"
	},
	{ "hr",
		"Ivica Koli\n"
		"Zlatko Sehanović\n"
		// "taos\n" bugfixes only, no actual translations
		"zvacet\n"
	},
	{ "hu",
		"Zsolt Bicskei\n"
		"Róbert Dancsó (dsjonny)\n"
		"Kálmán Kéménczy\n"
		"Zoltán Mizsei (miqlas)\n"
		"Bence Nagy\n"
		"Reznikov Sergei (Diver)\n"
		"Zoltán Szabó (Bird)\n"
	},
	{ "it",
		"Andrea Bernardi\n"
		"Dario Casalinuovo\n"
		"Francesco Franchina\n"
		"Michele Frau (zuMi)\n"
		"Lorenzo Frezza\n"
		"Mometto Nicola\n"
		"Michael Peppers\n"
		"davide.orsi\n"
	},
	{ "ja",
		"Satoshi Eguchi\n"
		"Shota Fukumori\n"
		"hiromu1996\n"
		"Hironori Ichimiya\n"
		"Jorge G. Mare (Koki)\n"
		"Takashi Murai\n"
		"nolze\n"
		"SHINTA\n"
		"thebowseat\n"
		"Hiroyuki Tsutsumi\n"
		"Hiromu Yakura\n"
		"The JPBE.net user group\n"
	},
	{ "lt",
		"Algirdas Buckus\n"
		"Simonas Kazlauskas\n" //nagisa
		"Rimas Kudelis\n"
	},
	{ "nb"
		"Klapaucius\n"
		"petterhj\n"
	},
	{ "nl",
		"Floris Kint\n" // FKint
		"Puck Meerburg\n" // puckipedia
		"Niels Sascha Reedijk\n" //nielx
		"Meanwhile\n"
		"bilstrong5\n"
		"lolacio\n"
		"madhusudansrg4\n"
		"michiel\n"
		"Sintendo\n"
		"Tagliano\n"
		// "taos\n" bugfixes only, no actual translations
		"wrench123456789\n"
	},
	{ "pl",
		"Szymon Barczak\n"
		"Przemysław Buczkowski\n"
		"Grzegorz Dąbrowski\n"
		"Hubert Hareńczyk\n" // Hubert
		"Kacper Kasper (KapiX)\n"
		"Krzysztof Miemiec\n"
		"Przemysław Pintal\n"
		"Artur Wyszyński\n"
		"flegmatyk\n"
	},
	{ "pt",
		"Marcos Alves (Xeon3D)\n"
		"Vasco Costa (gluon)\n"
		"Adriano Duarte\n"
		"Louis de M.\n"
		"Michael Vinícius de Oliveira (michaelvo)\n"
		// "taos\n" bugfixes only, no actual translations
	},
	{ "pt_BR",
		"Cabral Bandeira (beyraq)\n"
		"Adriano A. Duarte (Sri_Dhryko)\n"
		"Tiago Matos (tiagoms)\n"
		"Louis de M.\n"
		"Luis Otte\n"
		"Nadilson Santana (nadilsonsantana)\n"
	},
	{ "ro",
		"Victor Carbune\n"
		"Silviu Dureanu\n"
		"Alexsander Krustev\n"
		"Danca Monica\n"
		"Florentina Mușat\n"
		"Dragos Serban\n"
		"Hedeș Cristian Teodor\n"
		"Ivaylo Tsenkov\n"
		"Călinescu Valentin\n"
		"cipri\n"
	},
	{ "ru",
		"Dan Aller\n"
		"Tatyana Fursic (iceid)\n"
		"StoroZ Gneva\n"
		"Rodastahm Islamov (RISC)\n"
		"Eugene Katashov (mrNoisy)\n"
		"Pavel Kiryukhin\n"
		"Reznikov Sergei (Diver)\n"
		"Michael Smirnov\n"
		"Vladimir Vasilenko\n"
		"Siaržuk Žarski\n"
		"Ruskidecko\n"
		// "taos\n" bugfixes only, no actual translations
	},
	{ "sk",
		"Bachoru\n"
		"Ivan Masár\n"
	},
	{ "sl",
		"Matej Horvat\n"
	},
#if 0
	{ "sr",
		"Nikola Miljković\n"
	},
#endif
	{ "sv",
		"Patrik Gissberg\n"
		"Johan Holmberg\n"
		"Fredrik Modéen\n"
		"Jimmy Olsson (phalax)\n"
		"Jonas Sundström\n"
		"Victor Widell\n"
	},
#if 0
	{ "tr",
		"Hüseyin Aksu\n"
		"Halil İbrahim Azak\n"
		"Aras Ergus\n"
		"Enes Burhan Kuran\n"
		"Ali Rıza Nazlı\n"
	},
#endif
	{ "uk",
		"Mariya Pilipchuk\n"
		"Alex Rudyk (totish)\n"
		"Reznikov Sergei (Diver)\n"
		"Oleg Varunok\n"
		"axeller\n"
		"neiron13\n"
	},
	{ "zh",
		"Pengfei Han (kurain)\n"
		"Don Liu\n"
		"adcros\n"
		"dgy18787\n"
		"hlwork\n"
	}
};

#define kNumberOfTranslations (sizeof(kTranslations) / sizeof(Translation))

#define kCurrentMaintainers \
	"Ithamar R. Adema\n" \
	"Bruno G. Albuquerque\n" \
	"Stephan Aßmus\n" \
	"Stefano Ceccherini\n" \
	"Rudolf Cornelissen\n" \
	"Alexandre Deckner\n" \
	"Adrien Destugues\n" \
	"Oliver Ruiz Dorantes\n" \
	"Axel Dörfler\n" \
	"Jérôme Duval\n" \
	"Pawel Dziepak\n" \
	"René Gollent\n" \
	"Bryce Groff\n" \
	"Colin Günther\n" \
	"Jessica Hamilton\n" \
	"Julian Harnath\n" \
	"Fredrik Holmqvist\n" \
	"Philippe Houdoin\n" \
	"Ryan Leavengood\n" \
	"Andrew Lindesay\n" \
	"Michael Lotz\n" \
	"Matt Madia\n" \
	"Scott McCreary\n" \
	"David McPaul\n" \
	"Fredrik Modéen\n" \
	"Marcus Overhagen\n" \
	"Michael Pfeiffer\n" \
	"Joseph R. Prostko\n" \
	"François Revol\n" \
	"Philippe Saint-Pierre\n" \
	"Jonathan Schleifer\n" \
	"John Scipione\n" \
	"Alex Smith\n" \
	"Oliver Tappe\n" \
	"Gerasim Troeglazov\n" \
	"Alexander von Gluck IV\n" \
	"Ingo Weinhold\n" \
	"Alex Wilson\n" \
	"Artur Wyszyński\n" \
	"Clemens Zeidler\n" \
	"Siarzhuk Zharski\n" \
	"\n"

#define kPastMaintainers \
	"Andrew Bachmann\n" \
	"Salvatore Benedetto\n" \
	"Tyler Dauwalder\n" \
	"Daniel Furrer\n" \
	"Andre Alves Garzia\n" \
	"Karsten Heimrich\n" \
	"Erik Jaesler\n" \
	"Maurice Kalinowski\n" \
	"Euan Kirkhope\n" \
	"Marcin Konicki\n" \
	"Waldemar Kornewald\n" \
	"Thomas Kurschel\n" \
	"Brecht Machiels\n" \
	"Wim van der Meer\n" \
	"Frans Van Nispen\n" \
	"Adi Oanca\n" \
	"Michael Phipps\n" \
	"Niels Sascha Reedijk\n" \
	"David Reid\n" \
	"Hugo Santos\n" \
	"Alexander G. M. Smith\n" \
	"Andrej Spielmann\n" \
	"Jonas Sundström\n" \
	"Bryan Varner\n" \
	"Nathan Whitehorn\n" \
	"Michael Wilber\n" \
	"Jonathan Yoder\n" \
	"Gabe Yoder\n" \
	"\n"

#define kContributors \
	"Andrea Anzani\n" \
	"Sean Bartell\n" \
	"Hannah Boneß\n" \
	"Andre Braga\n" \
	"Michael Bulash\n" \
	"Bruce Cameron\n" \
	"Jean-Loïc Charroud\n" \
	"Greg Crain\n" \
	"Michael Davidson\n" \
	"David Dengg\n" \
	"John Drinkwater\n" \
	"Yongcong Du\n" \
	"Cian Duffy\n" \
	"Vincent Duvert\n" \
	"Mikael Eiman\n" \
	"Fredrik Ekdahl\n" \
	"Joshua R. Elsasser\n" \
	"Atis Elsts\n" \
	"Mark Erben\n" \
	"Christian Fasshauer\n" \
	"Andreas Färber\n" \
	"Janito Ferreira Filho\n" \
	"Pier Luigi Fiorini\n" \
	"Marc Flerackers\n" \
	"Michele Frau (zuMi)\n" \
	"Landon Fuller\n" \
	"Deyan Genovski\n" \
	"Pete Goodeve\n" \
	"Lucian Adrian Grijincu\n" \
	"Sean Healy\n" \
	"Andreas Henriksson\n" \
	"Matthijs Hollemans\n" \
	"Mathew Hounsell\n" \
	"Morgan Howe\n" \
	"Christophe Huriaux\n" \
	"Jian Jiang\n" \
	"Ma Jie\n" \
	"Carwyn Jones\n" \
	"Prasad Joshi\n" \
	"Vasilis Kaoutsis\n" \
	"James Kim\n" \
	"Shintaro Kinugawa\n" \
	"Jan Klötzke\n" \
	"Kurtis Kopf\n" \
	"Tomáš Kučera\n" \
	"Luboš Kulič\n" \
	"Elad Lahav\n" \
	"Anthony Lee\n" \
	"Santiago Lema\n" \
	"Raynald Lesieur\n" \
	"Oscar Lesta\n" \
	"Jerome Leveque\n" \
	"Freeman Lou\n" \
	"Brian Luft\n" \
	"Christof Lutteroth\n" \
	"Graham MacDonald\n" \
	"Jorge G. Mare (Koki)\n" \
	"Jan Matějek\n" \
	"Brian Matzon\n" \
	"Christopher ML Zumwalt May\n" \
	"Andrew McCall\n" \
	"Puck Meerburg\n" \
	"Nathan Mentley\n" \
	"Marius Middelthon\n" \
	"Marco Minutoli\n" \
	"Misza\n" \
	"Hamish Morrison\n" \
	"MrSiggler\n" \
	"Takashi Murai\n" \
	"Alan Murta\n" \
	"Raghuram Nagireddy\n" \
	"Kazuho Okui\n" \
	"Jeroen Oortwijn (idefix)\n" \
	"Pahtz\n" \
	"Michael Paine\n" \
	"Adrian Panasiuk\n" \
	"Aleksas Pantechovskis\n" \
	"Romain Picard\n" \
	"Francesco Piccinno\n" \
	"Peter Poláčik\n" \
	"David Powell\n" \
	"Jeremy Rand\n" \
	"Hartmut Reh\n" \
	"Daniel Reinhold\n" \
	"Sergei Reznikov\n" \
	"Chris Roberts\n" \
	"Samuel Rodríguez Pérez\n" \
	"Thomas Roell\n" \
	"Rafael Romo\n" \
	"Ralf Schülke\n" \
	"Zousar Shaker\n" \
	"Caitlin Shaw\n" \
	"Geoffry Song\n" \
	"Daniel Switkin\n" \
	"Atsushi Takamatsu\n" \
	"James Urquhart\n" \
	"Jason Vandermark\n" \
	"Sandor Vroemisse\n" \
	"Jürgen Wall\n" \
	"Denis Washington\n" \
	"Ulrich Wimboeck\n" \
	"Johannes Wischert\n" \
	"James Woodcock\n" \
	"Hong Yul Yang\n" \
	"Gerald Zajac\n" \
	"Łukasz Zemczak\n" \
	"JiSheng Zhang\n" \
	"Zhao Shuai\n"

#define kWebsiteTeam  \
	"Phil Greenway\n" \
	"Gavin James\n" \
	"Urias McCullough\n" \
	"Niels Sascha Reedijk\n" \
	"Joachim Seemer (Humdinger)\n" \
	"Jonathan Yoder\n" \
	"\n"


#endif // CREDITS_H
