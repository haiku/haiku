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
#if 0
	{ "bg",
		"Ognyan Valeri Angelov\n"
		"Росен Арабаджиев\n"
		"cssvb94\n"
		"naydef\n"
	},
#endif
	{ "ca",
		"David Medina\n" // Pootle: David MP
		"Paco Rivière\n"
		"jare\n"
	},
	{ "zh",
		"Dong Guangyu\n"
		"Pengfei Han (kurain)\n"
		"Don Liu\n"
		"adcros\n"
		"blumia\n"
		"cirno\n"
		"dgy18787\n"
		"hlwork\n"
		"raphino\n"
	},
	{ "hr",
		"Ivica Kolić\n" // Pootle: zvacet
		"Zlatko Sehanović\n"
		// "taos\n" bugfixes only, no actual translations
	},
#if 0
	{ "cs",
		"aXeton\n"
		"Ivan Masár\n"
		"Pavel Drotár\n"
		"Martin Janiczek\n"
		"Matěj Kocián\n"
		"zafan\n"
	},
#endif
	{ "da",
		"Brian Matzon\n"
		"Kristian Poul Herkild\n"
		"KapiX\n"
		"sylvester\n"
		"scootergrisen\n"
	},
	{ "nl",
		"Floris Kint\n" // FKint
		"Puck Meerburg\n" // puckipedia
		"Niels Sascha Reedijk\n" //nielx
		"Begasus\n"
		"bilstrong5\n"
		"lolacio\n"
		"mauritslamers\n"
		"madhusudansrg4\n"
		"Meanwhile\n"
		"michiel\n"
		"noaccount\n"
		"Sintendo\n"
		"Tagliano\n"
		// "taos\n" bugfixes only, no actual translations
		"wrench123456789\n"
	},
#if 0
	{ "en_ca",
		"Edwin Amsler\n"
		"infamy\n"
		"Simon South\n"
	},
#endif
	{ "en_gb",
		"Harsh Vardhan\n"
		"Humdinger\n"
		"Jessica_Lily\n"
		"Richie Nyhus\n"
	},
	{ "eo",
		"Alexey Burshtein (Hitech)\n"
		"Travis D. Reed (Dancxjo)\n"
		"CodeWeaver\n"
		"jaidedpts\n"
		"jmairboeck\n"
		"kio\n"
		"kojoto\n"
		"Lutrulo\n"
		"miguel-1.mx\n"
		"Neber\n"
		"ora_birdo\n"
		"SandwichDoktoro\n"
		"sikmir\n"
		"Teashrock\n"
		"Va\n"
	},
	{ "fi",
		"Jorma Karvonen (Karvjorm)\n"
		"Jaakko Leikas (Garjala)\n"
		"Tommi Nieminen (SuperOscar)\n"
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
		"Florent Revest\n"
		"Harsh Vardhan\n"
		"abda11ah\n"
		"Adeimantos\n"
		"Blazkowitz\n"
		"Loïc\n"
		"roptat\n"
		"Starchaser\n"
		"Wabouz\n"
		"JozanLeClerc\n"
		"Krevar\n"
		"sunazerty\n"
	},
	{ "fur",
		"Fabio Tomat\n"
	},
	{ "de",
		"Colin Günther\n"
		"Mirko Israel\n"
		"Christian Morgenroth\n"
		"Aleksas Pantechovskis\n"
		"Joachim Seemer (Humdinger)\n"
		"Matthias Spreiter\n"
		"Ivaylo Tsenkov\n"
		"Atalanttore\n"
		"leszek\n"
		"svend\n"
		"taos\n"
		"FabianR\n"
		"HonkXL\n"
	},
	{ "el",
		"Γιάννης Κωνσταντινίδης [Giannis Konstantinidis] (giannisk)\n"
		"Βαγγέλης Μαμαλάκης [Vaggelis Mamalakis]\n"
		"Άλεξ-Π. Νάτσιος [Alex-P. Natsios] (Drakevr)\n"
		"Efstathios Iosifidis\n"
		"Edward Pasenidis\n"
		"Jim Spentzos\n"
		"Panagiotis Vasilopoulos (AlwaysLivid)\n"
		"blu.256\n"
		"JamesSP472\n"
		"hxze\n"
		"Greek_Ios_Hack\n"
		"greekboy\n"
		"vasper\n"
	},
#if 0
	{ "hi",
		"Abhishek Arora\n"
		"Dhruwat Bhagat\n"
		"Jayneil Dalal\n"
		"Atharva Lath\n"
		"Harsh Vardhan\n"
		"unitedroad\n"
	},
#endif
	{ "hu",
		"Zsolt Bicskei\n"
		"Róbert Dancsó (dsjonny)\n"
		"Kálmán Kéménczy\n"
		"Zoltán Mizsei (miqlas)\n"
		"Bence Nagy\n"
		// "Reznikov Sergei (Diver)\n" bugfixes only
		"Zoltán Szabó (Bird)\n"
		"Zoltán Szatmáry\n"
		"johnny_b\n"
	},
	{ "id",
		"Raefal Dhia\n"
		"Henry Guzman\n"
		"Dityo Nurasto\n"
		"Bre.haviours\n"
		"iyank4\n"
		"mazbrili\n"
		"muriirum\n"
	},
	{ "it",
		"Gabriele Baldassarre\n"
		"Andrea Bernardi\n"
		"Pavlo Bvrda\n"
		"Dario Casalinuovo\n"
		"Francesco Franchina\n"
		"Michele Frau (zuMi)\n"
		"Lorenzo Frezza\n"
		"Mometto Nicola\n"
		"Michael Peppers\n"
		"Fabio Tomat\n"
		"Daniele Tosti\n"
		"davide.orsi\n"
		"fabiusp98\n"
		"TropinotoHirto\n"
		"valzant\n"
		"JackBurton\n"
		"errata_corrige\n"
		"pharaone\n"
	},
	{ "ja",
		"Satoshi Eguchi\n"
		"Shota Fukumori\n"
		"Hironori Ichimiya\n"
		"Jorge G. Mare (Koki)\n"
		"Takashi Murai\n"
		"Hiroyuki Tsutsumi\n"
		"Hiromu Yakura\n"
		"hiromu1996\n"
		"nolze\n"
		"SHINTA\n"
		"thebowseat\n"
		"The JPBE.net user group\n"
	},
#if 0
	{ "ko",
		"soul.lee\n"
	},
#endif
	{ "lt",
		"Algirdas Buckus\n"
		"Simonas Kazlauskas\n" //nagisa
		"Rimas Kudelis\n"
	},
#if 0
	{ "fa",
		"Mehran Rahbardar\n"
	},
	{ "mi",
		"Rob Judd\n"
	},
	{ "nb",
		"Klapaucius\n"
		"petterhj\n"
	},
#endif
	{ "pl",
		"Szymon Barczak\n"
		"Przemysław Buczkowski\n"
		"Grzegorz Dąbrowski\n"
		"Hubert Hareńczyk\n" // Hubert
		"Kacper Kasper (KapiX)\n"
		"Dariusz Knociński\n"
		"Krzysztof Miemiec\n"
		"Przemysław Pintal\n"
		"Artur Wyszyński\n"
		"flegmatyk\n"
		"rausman\n"
		"stasinek\n"
		"zzzzzzzzz\n"
		"arckat\n"
		"nawordar\n"
		"wielb\n"
	},
	{ "pt",
		"Marcos Alves (Xeon3D)\n"
		"Vasco Costa (gluon)\n"
		"Victor Domingos (victordomingos)\n"
		"Adriano Duarte\n"
		"Louis de M.\n"
		"Ricardo Simões\n"
		"pedrothegameroficialtm\n"
		"zeru\n"
		"jcfb\n"
		// "taos\n" bugfixes only, no actual translations
	},
	{ "pt_BR",
		"Cabral Bandeira (beyraq)\n"
		"Victor Domingos (victordomingos)\n"
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
		"Yurii Zamotailo\n" // Pootle: IaH
		"Алексей Мехоношин\n"
		"Ruskidecko\n"
		"Snowfire\n"
		"solarcold\n"
		"i-Demon-i\n"
		"Roskin\n"
		"TK-313\n"
		"iimetra\n"
		"sikmir\n"
		// "taos\n" bugfixes only, no actual translations
	},
	{ "sk",
		"Bachoru\n"
		"Ivan Masár\n"
	},
#if 0
	{ "sl",
		"Matej Horvat\n"
	},
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
		"Calebin\n"
		"Jose64141\n"
		"cafeina\n"
		"dhijazo\n"
		"jma_sp\n"
		"jonatanpc8\n"
		"mikelcaz\n"
		"quique\n"
		"snunezcr\n"
		"xgrimator\n"
		"zeroxp\n"
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
		"Forza\n"
	},
	{ "th",
		"Sompon Chanrit\n"
	},
	{ "tr",
		"Hüseyin Aksu\n"
		"Halil İbrahim Azak\n"
		"Aras Ergus\n"
		"Mustafa Gökay\n"
		"Enes Burhan Kuran\n"
		"Ali Rıza Nazlı\n"
		"Anıl Özbek\n"
		"Emir Sarı\n" // bitigchi
		"Sinan Talebi\n"
		"csakirt\n"
		"Hezarfen\n"
		"interlude\n"
		"Kardanadam\n"
		"ocingiler\n"
		"yakup\n"
		"ctasan\n"
	},
	{ "uk",
		"Pavlo Bvrda\n"
		"Mariya Pilipchuk\n"
		"Alex Rudyk (totish)\n"
		// "Reznikov Sergei (Diver)\n" bugfixes only
		"Oleg Varunok\n"
		"Yurii Zamotailo\n" // Pootle: IaH
		"axeller\n"
		"neiron13\n"
		"mayk77\n"
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
	"Rajagopalan Gangadharan\n" \
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
	"Panagiotis Vasilopoulos (AlwaysLivid)\n" \
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
