/*
 * Copyright 2005-2020, Haiku, Inc.
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
		"Росен Арабаджиев\n"
		"cssvb94\n"
		"naydef\n"
	},
	{ "ca",
		"Paco Rivière\n"
	},
	{ "zh",
		"Dong Guangyu\n"
		"Pengfei Han (kurain)\n"
		"Don Liu\n"
		"adcros\n"
		"cirno\n"
		"dgy18787\n"
		"hlwork\n"
	},
	{ "hr",
		"Ivica Koli\n"
		"Zlatko Sehanović\n"
		// "taos\n" bugfixes only, no actual translations
		"zvacet\n"
	},
	{ "cs",
		"aXeton\n"
		"Ivan Masár\n"
		"Pavel Drotár\n"
		"Martin Janiczek\n"
		"Matěj Kocián\n"
		"zafan\n"
	},
	{ "da",
		"KapiX\n"
		"Brian Matzon\n"
		"Kristian Poul Herkild\n"
		"sylvester\n"
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
	{ "en_ca",
		"Edwin Amsler\n"
		"infamy\n"
		"Simon South\n"
	},
	{ "en_gb",
		"Harsh Vardhan\n"
		"Humdinger\n"
		"Jessica_Lily\n"
		"Richie Nyhus\n"
	},
	{ "eo",
		"Travis D. Reed (Dancxjo)\n"
		"kojoto\n"
	},
	{ "fi",
		"Jorma Karvonen (Karvjorm)\n"
		"Jaakko Leikas (Garjala)\n"
		"Tommi Nieminen (SuperOscar)\n"
		"Niels Sascha Reedijk\n"
		"Slavi Stefanov Sotirov\n"
		"nortti\n"
	},
	{ "fr",
		"Edwin Amsler\n"
		"Yannick Barbel\n"
		"Jean-Loïc Charroud\n"
		"Adrien Destugues (PulkoMandy)\n"
		"Jérôme Duval (korli)\n"
		"François Revol (mmu_man)\n"
		"Niels Sascha Reedijk\n"
		"Florent Revest\n"
		"Harsh Vardhan\n"
		"abda11ah\n"
		"Adeimantos\n"
		"Blazkowitz\n"
		"Loïc\n"
		"roptat\n"
		"Starchaser\n"
		"Wabouz\n"
	},
	{ "fur",
		"Fabio Tomat\n"
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
		"Efstathios Iosifidis\n"
		"Jim Spentzos\n"
		"JamesSP472\n"
		"vasper\n"
	},
	{ "hi",
		"Abhishek Arora\n"
		"Dhruwat Bhagat\n"
		"Jayneil Dalal\n"
		"Atharva Lath\n"
		"Harsh Vardhan\n"
		"unitedroad\n"
	},
	{ "hu",
		"Zsolt Bicskei\n"
		"Róbert Dancsó (dsjonny)\n"
		"Kálmán Kéménczy\n"
		"Zoltán Mizsei (miqlas)\n"
		"Bence Nagy\n"
		// "Reznikov Sergei (Diver)\n" bugfixes only
		"Zoltán Szabó (Bird)\n"
	},
	{ "id",
		"Raefal Dhia\n"
		"Henry Guzman\n"
		"iyank4\n"
		"mazbrili\n"
		"nurasto\n"
	},
	{ "it",
		"Andrea Bernardi\n"
		"Pavlo Bvrda\n"
		"Dario Casalinuovo\n"
		"Francesco Franchina\n"
		"Michele Frau (zuMi)\n"
		"Lorenzo Frezza\n"
		"Mometto Nicola\n"
		"Michael Peppers\n"
		"Daniele Tosti\n"
		"davide.orsi\n"
		"fabiusp98\n"
		"TheClue\n"
		"TropinotoHirto\n"
		"valzant\n"
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
	{ "ko",
		"soul.lee\n"
	},
	{ "lt",
		"Algirdas Buckus\n"
		"Simonas Kazlauskas\n" //nagisa
		"Rimas Kudelis\n"
	},
#if 0
	{ "fa",
		"Mehran Rahbardar\n"
	},
#endif
	{ "mi",
		"Rob Judd\n"
	},
	{ "nb",
		"Klapaucius\n"
		"petterhj\n"
	},
	{ "pl",
		"Dariusz Knociński\n"
		"Szymon Barczak\n"
		"Przemysław Buczkowski\n"
		"Grzegorz Dąbrowski\n"
		"Hubert Hareńczyk\n" // Hubert
		"Kacper Kasper (KapiX)\n"
		"Krzysztof Miemiec\n"
		"Przemysław Pintal\n"
		"Artur Wyszyński\n"
		"flegmatyk\n"
		"rausman\n"
		"stasinek\n"
		"zzzzzzzzz\n"
	},
	{ "pt",
		"Marcos Alves (Xeon3D)\n"
		"Vasco Costa (gluon)\n"
		"Victor Domingos\n"
		"Adriano Duarte\n"
		"Louis de M.\n"
		"pedrothegameroficialtm\n"
		"zeru\n"
		// "taos\n" bugfixes only, no actual translations
	},
	{ "pt_BR",
		"Cabral Bandeira (beyraq)\n"
		"Victor Domingos\n"
		"Adriano A. Duarte (Sri_Dhryko)\n"
		"Wallace Maia\n"
		"Tiago Matos (tiagoms)\n"
		"Louis de M.\n"
		"Michael Vinícius de Oliveira (michaelvo)\n"
		"Luis Otte\n"
		"Nadilson Santana (nadilsonsantana)\n"
		"dsaito\n"
	},
	{ "ro",
		"Victor Carbune\n"
		"Silviu Dureanu\n"
		"Alexsander Krustev\n"
		"Danca Monica\n"
		"Florentina Mușat\n" // Emrys
		"Dragos Serban\n"
		"Hedeș Cristian Teodor\n"
		"Ivaylo Tsenkov\n"
		"Călinescu Valentin\n"
		"cipri\n"
		"valzant\n"
	},
	{ "ru",
		"Dan Aller\n"
		"Luka Andjelkovic\n"
		"Tatyana Fursic (iceid)\n"
		"StoroZ Gneva\n"
		"Rodastahm Islamov (RISC)\n"
		"Eugene Katashov (mrNoisy)\n"
		"Pavel Kiryukhin\n"
		"Rimas Kudelis\n"
		"Reznikov Sergei (Diver)\n"
		"Michael Smirnov\n"
		"Sergei Sorokin\n"
		"Vladimir Vasilenko\n"
		"Siaržuk Žarski\n"
		"Алексей Мехоношин\n"
		"Ruskidecko\n"
		"Snowfire\n"
		"solarcold\n"
		"i-Demon-i\n"
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
		"Luka Andjelkovic\n"
		"Nikola Miljkovic\n"
	},
#endif
	{ "es",
		"Pedro Arregui\n"
		"José Antonio Barranquero\n"
		"Zola Bridges\n"
		"Nicolás C (CapitanPico)\n"
		"Oscar Carballal (oscarcp)\n"
		"Dario de la Cruz\n"
		"Francisco Gómez\n"
		"Luis Gustavo Lira\n"
		"Louis de M.\n"
		"Victor Madariaga\n"
		"Remy Matos\n"
		"César Ortiz Pantoja (ccortiz)\n"
		"Miguel Zúñiga González (miguel~1.mx)\n"
		"Virgilio Leonardo Ruilova\n"
		"algrun\n"
		"asak_lemon\n"
		"c-sanchez\n"
		"EdiNOS\n"
		"espectalll123\n"
		"fatigatti\n"
		"grifus\n"
		"jdari\n"
		"jjpx\n"
		"mpescador\n"
		"OscarL\n"
		"ruilovacastillo\n"
		"valzant\n"
		"zerabat\n"
		// "taos\n" bugfixes only, no actual translations
	},
	{ "sv",
		"Patrik Gissberg\n"
		"Johan Holmberg\n"
		"Theo Knez\n"
		"Fredrik Modéen\n"
		"Jimmy Olsson (phalax)\n"
		"Jonas Sundström\n"
		"Anders Trobäck\n"
		"Victor Widell\n"
	},
	{ "tr",
		"Hüseyin Aksu\n"
		"Halil İbrahim Azak\n"
		"Aras Ergus\n"
		"Mustafa Gökay\n"
		"Enes Burhan Kuran\n"
		"Ali Rıza Nazlı\n"
		"Anıl Özbek\n"
		"Emir Sarı\n"
		"Sinan Talebi\n"
		"csakirt\n"
		"Hezarfen\n"
		"interlude\n"
		"Kardanadam\n"
		"ocingiler\n"
		"yakup\n"
	},
	{ "uk",
		"Pavlo Bvrda\n"
		"Mariya Pilipchuk\n"
		"Alex Rudyk (totish)\n"
		// "Reznikov Sergei (Diver)\n" bugfixes only
		"Oleg Varunok\n"
		"axeller\n"
		"neiron13\n"
	},
};

#define kNumberOfTranslations (sizeof(kTranslations) / sizeof(Translation))

#define kCurrentMaintainers \
	"Kyle Ambroff-Kao\n" \
	"Stephan Aßmus\n" \
	"Augustin Cavalier\n" \
	"Stefano Ceccherini\n" \
	"Adrien Destugues\n" \
	"Axel Dörfler\n" \
	"Jérôme Duval\n" \
	"René Gollent\n" \
	"Jessica Hamilton\n" \
	"Julian Harnath\n" \
	"Brian Hill\n" \
	"Fredrik Holmqvist\n" \
	"Philippe Houdoin\n" \
	"Kacper Kasper\n" \
	"Ryan Leavengood\n" \
	"Andrew Lindesay\n" \
	"Michael Lotz\n" \
	"Scott McCreary\n" \
	"Puck Meerburg\n" \
	"Fredrik Modéen\n" \
	"Joseph R. Prostko\n" \
	"Niels Sascha Reedijk\n" \
	"François Revol\n" \
	"Philippe Saint-Pierre\n" \
	"Jonathan Schleifer\n" \
	"John Scipione\n" \
	"Gerasim Troeglazov\n" \
	"Alexander von Gluck IV\n" \
	"Ingo Weinhold\n" \
	"\n"

#define kPastMaintainers \
	"Ithamar R. Adema\n" \
	"Bruno G. Albuquerque\n" \
	"Andrew Bachmann\n" \
	"Salvatore Benedetto\n" \
	"Dario Casalinuovo\n" \
	"Rudolf Cornelissen\n" \
	"Tyler Dauwalder\n" \
	"Alexandre Deckner\n" \
	"Oliver Ruiz Dorantes\n" \
	"Pawel Dziepak\n" \
	"Daniel Furrer\n" \
	"Andre Alves Garzia\n" \
	"Bryce Groff\n" \
	"Colin Günther\n" \
	"Karsten Heimrich\n" \
	"Erik Jaesler\n" \
	"Maurice Kalinowski\n" \
	"Euan Kirkhope\n" \
	"Marcin Konicki\n" \
	"Waldemar Kornewald\n" \
	"Thomas Kurschel\n" \
	"Brecht Machiels\n" \
	"Matt Madia\n" \
	"David McPaul\n" \
	"Wim van der Meer\n" \
	"Hamish Morrison\n" \
	"Michael Pfeiffer\n" \
	"Frans Van Nispen\n" \
	"Adi Oanca\n" \
	"Marcus Overhagen\n" \
	"Michael Phipps\n" \
	"David Reid\n" \
	"Hugo Santos\n" \
	"Alex Smith\n" \
	"Alexander G. M. Smith\n" \
	"Andrej Spielmann\n" \
	"Jonas Sundström\n" \
	"Oliver Tappe\n" \
	"Bryan Varner\n" \
	"Nathan Whitehorn\n" \
	"Michael Wilber\n" \
	"Alex Wilson\n" \
	"Artur Wyszyński\n" \
	"Jonathan Yoder\n" \
	"Clemens Zeidler\n" \
	"Siarzhuk Zharski\n" \
	"\n"

#define kTestingTeam \
	"luroh\n" \
	"Robert Mercer (Vidrep)\n" \
	"Sergei Reznikov\n" \
	"\n"

#define kContributors \
	"Pascal R. G. Abresch\n" \
	"Andrea Anzani\n" \
	"Sean Bartell\n" \
	"Sambuddha Basu\n" \
	"Hannah Boneß\n" \
	"Andre Braga\n" \
	"Michael Bulash\n" \
	"Bruce Cameron\n" \
	"Jean-Loïc Charroud\n" \
	"Ilya Chugin (X512)\n" \
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
	"Markus Himmel\n" \
	"Matthijs Hollemans\n" \
	"Mathew Hounsell\n" \
	"Morgan Howe\n" \
	"Christophe Huriaux\n" \
	"Jian Jiang\n" \
	"Ma Jie\n" \
	"Carwyn Jones\n" \
	"Prasad Joshi\n" \
	"Vasilis Kaoutsis\n" \
	"Preetpal Kaur\n" \
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
	"Jan Matějek\n" \
	"Brian Matzon\n" \
	"Christopher ML Zumwalt May\n" \
	"Andrew McCall\n" \
	"Nathan Mentley\n" \
	"Marius Middelthon\n" \
	"Marco Minutoli\n" \
	"Misza\n" \
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
	"Gabe Yoder\n" \
	"Gerald Zajac\n" \
	"Łukasz Zemczak\n" \
	"JiSheng Zhang\n" \
	"Zhao Shuai\n"

#define kWebsiteTeam  \
	"Augustin Cavalier\n" \
	"Niels Sascha Reedijk\n" \
	"Joachim Seemer (Humdinger)\n" \
	"\n"

#define kPastWebsiteTeam  \
	"Phil Greenway\n" \
	"Gavin James\n" \
	"Jorge G. Mare (Koki)\n" \
	"Urias McCullough\n" \
	"Richie Nyhus\n" \
	"Jonathan Yoder\n" \
	"\n"


#endif // CREDITS_H
