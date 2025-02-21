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
		"Adolfo Jayme Barrientos\n"
		"David Medina\n" // Pootle: David MP
		"Paco Rivière\n"
		"jare\n"
	},
	{ "zh_Hans",
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
	{ "zh_Hant",
		"daniel03663\n"
		"edouardlicn\n"
	},
	{ "hr",
		"Ivica Kolić\n" // Pootle: zvacet
		"Zlatko Sehanović\n"
		// "taos\n" bugfixes only, no actual translations
	},
	{ "cs",
		"aXeton\n"
		"hearse\n"
		"hor411\n"
		"huh2\n"
		"Ivan Masár\n"
		"jpz\n"
		"jsvoboda\n"
		"Pavel Drotár\n"
		"Martin Janiczek\n"
		"MartinK\n"
		"Matěj Kocián\n"
		"mirek\n" // Pootle: mirek AT zebrak.net
		"nkin\n"
		"vitekpin\n"
		"zafan\n"
	},
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
		"nickbouwhuis\n"
		"noaccount\n"
		"Sintendo\n"
		"SomeoneFromBelgium\n"
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
		"Anatolij\n"
		"Travis D. Reed (Dancxjo)\n"
		"CodeWeaver\n"
		"jaidedpts\n"
		"Jaidyn Ann\n"
		"JakubFabijan\n"
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
		"farvardin\n"
		"JeremyF\n"
		"JozanLeClerc\n"
		"Krevar\n"
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
		"FabianR\n"
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
		"Panagiotis \"Ivory\" Vasilopoulos (n0toose)\n"
		"blu.256\n"
		"JamesSP472\n"
		"hxze\n"
		"Greek_Ios_Hack\n"
		"greekboy\n"
		"Mavridis Philippe\n"
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
		"Czirok Attila\n"
		// "Reznikov Sergei (Diver)\n" bugfixes only
		"johnny_b\n"
		"ViktorVarga\n"
		"Zoltán Szabó (Bird)\n"
		"Zoltán Szatmáry\n"
	},
	{ "id",
		"Raefal Dhia\n" // Pootle: raefaldhia
		"Henry Guzman\n"
		"Dityo Nurasto\n"
		"Bre.haviours\n"
		"iyank4\n"
		"mazbrili\n"
		"muriirum\n"
	},
	{ "it",
		"Andrea Bernardi\n"
		"andrea77\n"
		"Daniele Tosti\n"
		"Dario Casalinuovo\n"
		"davide.orsi\n"
		"errata_corrige\n"
		"fabiusp98\n"
		"Gabriele Baldassarre\n"
		"JackBurton\n"
		"Fabio Tomat\n"
		"fabiusp98\n"
		"Francesco Franchina\n"
		"Lorenzo Frezza\n"
		"Michael Peppers\n"
		"Michele Frau (zuMi)\n"
		"Mometto Nicola\n"
		"Pavlo Bvrda\n"
		"pharaone\n"
		"TropinotoHirto\n"
		"valzant\n"
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
	{ "ko",
		"Jinuk Jung\n"
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
	{ "mi",
		"Rob Judd\n"
	},
	{ "nb",
		"Klapaucius\n"
		"petterhj\n"
	},
#endif
	{ "pl",
		"arckat\n"
		"Artur Wyszyński\n"
		"balu\n"
		"Dariusz Knociński\n"
		"FabijanJakub\n"
		"flegmatyk\n"
		"Grzegorz Dąbrowski\n"
		"Hubert Hareńczyk\n" // Hubert
		"i4096\n"
		"Kacper Kasper (KapiX)\n"
		"Krzysztof Miemiec\n"
		"nawordar\n"
		"nerd\n"
		"Przemysław Buczkowski\n"
		"Przemysław Pintal\n"
		"Qu1\n"
		"rausman\n"
		"sech1p\n"
		"Szymon Barczak\n"
		"stasinek\n"
		"wielb\n"
		"zzzzzzzzz\n"
	},
	{ "pt",
		"Adriano A. Duarte (Sri_Dhryko)\n"
		"bemagiw306\n"
		"emanuel\n"
		"hugok\n"
		"Imivorn\n"
		"jcfb\n"
		"Louis de M.\n"
		"Marcos Alves (Xeon3D)\n"
		"pedrothegameroficialtm\n"
		"Ricardo Simões\n"
		"Vasco Costa (gluon)\n"
		"Victor Domingos (victordomingos)\n"
		"zeru\n"
		// "taos\n" bugfixes only, no actual translations
	},
	{ "pt_BR",
		"Adriano A. Duarte (Sri_Dhryko)\n"
		"Anderson Rodrigues\n"
		"Andrei Torres\n"
		"Cabral Bandeira (beyraq)\n"
		"dsaito\n"
		"humbertocsjr\n"
		"Louis de M.\n"
		"Luis Otte\n"
		"Michael Vinícius de Oliveira (michaelvo)\n"
		"Nadilson Santana (nadilsonsantana)\n"
		"oddybr\n"
		"renanbirck\n"
		"Tiago Matos (tiagoms)\n"
		"Victor Domingos (victordomingos)\n"
		"Wallace Maia\n"
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
		"Alexey Burshtein (Hitech)\n"
		"bitigchi\n"
		"Dan Aller\n"
		"erhoof\n"
		"Eugene Katashov (mrNoisy)\n"
		"i-Demon-i\n"
		"iimetra\n"
		"Luka Andjelkovic\n"
		"Michael Smirnov\n"
		"Oleg Feoktistov\n"
		"Pavel Kiryukhin\n"
		"Reznikov Sergei (Diver)\n"
		"Rimas Kudelis\n"
		"Rodastahm Islamov (RISC)\n"
		"Roskin\n"
		"Ruskidecko\n"
		"Sergei Sorokin\n"
		"Siaržuk Žarski\n"
		"sikmir\n"
		"Snowfire\n"
		"solarcold\n"
		"Sosed\n"
		"StoroZ Gneva\n"
		"Tatyana Fursic (iceid)\n"
		"TK-313\n"
		"Va Miluŝnikov\n"
		"Vladimir Vasilenko\n"
		"Yurii Zamotailo\n" // Pootle: IaH
		"Yury\n"
		"Алексей Мехоношин\n"
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
		"Adolfo Jayme Barrientos\n"
		"AlanBur\n"
		"algrun\n"
		"asak_lemon\n"
		"bru\n"
		"c-sanchez\n"
		"cafeina\n"
		"Calebin\n"
		"César Ortiz Pantoja (ccortiz)\n"
		"Dario de la Cruz\n"
		"dhijazo\n"
		"EdiNOS\n"
		"espectalll123\n"
		"fatigatti\n"
		"Francisco Gómez\n"
		"grifus\n"
		"jdari\n"
		"jjpx\n"
		"jma_sp\n"
		"jonatanpc8\n"
		"José Antonio Barranquero\n"
		"Jose64141\n"
		"jzafrag\n"
		"Louis de M.\n"
		"Luis Gustavo Lira\n"
		"ManuelMegias\n"
		"Miguel Zúñiga González (miguel~1.mx)\n"
		"mikelcaz\n"
		"mpescador\n"
		"Nicolás C (CapitanPico)\n"
		"Oscar Carballal (oscarcp)\n"
		"OscarL\n"
		"Pedro Arregui\n"
		"Parodper\n"
		"Rubén Tomás\n"
		"quique\n"
		"Remy Matos\n"
		"ruilovacastillo\n"
		"snunezcr\n"
		"valenxia\n"
		"valzant\n"
		"Victor Madariaga\n"
		"Virgilio Leonardo Ruilova\n"
		"xgrimator\n"
		"zerabat\n"
		"zeroxp\n"
		"Zola Bridges\n"
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
		"Chaiwit Phonkhen\n"
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
		"husaral\n"
	},
	{ "uk",
		"Alex Rudyk (totish)\n"
		"Alexey Lugin\n"
		"axeller\n"
		// "Reznikov Sergei (Diver)\n" bugfixes only
		"Mariya Pilipchuk\n"
		"mayk77\n"
		"neiron13\n"
		"Oleg Varunok\n"
		"Pavlo Bvrda\n"
		"Yurii Zamotailo\n" // Pootle: IaH
	},
};

#define kNumberOfTranslations (sizeof(kTranslations) / sizeof(Translation))

#define kCurrentMaintainers \
	"Pascal R. G. Abresch\n" \
	"Máximo Castañeda\n" \
	"Augustin Cavalier\n" \
	"Stefano Ceccherini\n" \
	"Rudolf Cornelissen\n" \
	"Adrien Destugues\n" \
	"Axel Dörfler\n" \
	"Jérôme Duval\n" \
	"Alexander von Gluck IV\n" \
	"René Gollent\n" \
	"Jessica Hamilton\n" \
	"Fredrik Holmqvist\n" \
	"David Karoly\n" \
	"Kacper Kasper\n" \
	"Ryan Leavengood\n" \
	"Andrew Lindesay\n" \
	"Michael Lotz\n" \
	"Scott McCreary\n" \
	"Puck Meerburg\n" \
	"Niels Sascha Reedijk\n" \
	"François Revol\n" \
	"Jonathan Schleifer\n" \
	"John Scipione\n" \
	"Joachim Seemer (Humdinger)\n" \
	"Gerasim Troeglazov\n" \
	"\n"

#define kPastMaintainers \
	"Ithamar R. Adema\n" \
	"Bruno G. Albuquerque\n" \
	"Kyle Ambroff-Kao\n" \
	"Stephan Aßmus\n" \
	"Andrew Bachmann\n" \
	"Salvatore Benedetto\n" \
	"Dario Casalinuovo\n" \
	"Tyler Dauwalder\n" \
	"Alexandre Deckner\n" \
	"Oliver Ruiz Dorantes\n" \
	"Pawel Dziepak\n" \
	"Daniel Furrer\n" \
	"Andre Alves Garzia\n" \
	"Bryce Groff\n" \
	"Colin Günther\n" \
	"Julian Harnath\n" \
	"Karsten Heimrich\n" \
	"Brian Hill\n" \
	"Philippe Houdoin\n" \
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
	"Fredrik Modéen\n" \
	"Hamish Morrison\n" \
	"Frans Van Nispen\n" \
	"Adi Oanca\n" \
	"Marcus Overhagen\n" \
	"Michael Pfeiffer\n" \
	"Michael Phipps\n" \
	"Joseph R. Prostko\n" \
	"David Reid\n" \
	"Philippe Saint-Pierre\n" \
	"Hugo Santos\n" \
	"Alex Smith\n" \
	"Alexander G. M. Smith\n" \
	"Andrej Spielmann\n" \
	"Jonas Sundström\n" \
	"Oliver Tappe\n" \
	"Bryan Varner\n" \
	"Ingo Weinhold\n" \
	"Nathan Whitehorn\n" \
	"Michael Wilber\n" \
	"Alex Wilson\n" \
	"Artur Wyszyński\n" \
	"Jonathan Yoder\n" \
	"Clemens Zeidler\n" \
	"Siarzhuk Zharski\n" \
	"Janus\n" \
	"\n"

#define kTestingTeam \
	"luroh\n" \
	"Robert Mercer (Vidrep)\n" \
	"Sergei Reznikov\n" \
	"\n"

#define kContributors \
	"Andrea Anzani\n" \
	"Sean Bartell\n" \
	"Sambuddha Basu\n" \
	"Shubham Bhagat\n" \
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
	"Suhel Mehta\n" \
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
	"Niklas Poslovski\n" \
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
	"Panagiotis \"Ivory\" Vasilopoulos (n0toose)\n" \
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
