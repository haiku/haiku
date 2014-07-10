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
	{ "ar",
		"Khaled Berraoui (khallebal)\n"
		"Kendhia\n"
	},
	{ "be",
		"Michael Bulash\n"
	},
	{ "bg",
		"Ognyan Valeri Angelov\n"
		"cssvb94\n"
	},
	{ "cs",
		"Pavel Drotár\n"
		"Matěj Kocián\n"
	},
	{ "da",
		"Brian Matzon\n"
	},
	{ "de",
		"Colin Günther\n"
		"Mirko Israel\n"
		"leszek\n"
		"Christian Morgenroth\n"
		"Aleksas Pantechovskis\n"
		"Joachim Seemer (Humdinger)\n"
		"Matthias Spreiter\n"
		"Ivaylo Tsenkov\n"
		"svend\n"
	},
	{ "el",
		"Γιάννης Κωνσταντινίδης [Giannis Konstantinidis] (giannisk)\n"
		"Βαγγέλης Μαμαλάκης [Vaggelis Mamalakis]\n"
		"Άλεξ-Π. Νάτσιος [Alex-P. Natsios] (Drakevr)\n"
	},
	{ "eo",
		"Travis D. Reed (Dancxjo)\n"
	},
	{ "es",
		"Pedro Arregui\n"
		"Zola Bridges\n"
		"Nicolás C (CapitanPico)\n"
		"Oscar Carballal (oscarcp)\n"
		"Miguel Zúñiga González (miguel~1.mx)\n"
		"Luis Gustavo Lira\n"
		"Victor Madariaga\n"
		"César Ortiz Pantoja (ccortiz)\n"
	},
	{ "fi",
		"Jorma Karvonen (Karvjorm\n"
		"Jaakko Leikas (Garjala)\n"
		"Slavi Stefanov Sotirov\n"
	},
	{ "fr",
		"Jean-Loïc Charroud\n"
		"Adrien Destugues (PulkoMandy)\n"
		"Florent Revest\n"
		"Harsh Vardhan\n"
	},
	{ "hi",
		"Abhishek Arora\n"
		"Dhruwat Bhagat\n"
		"Jayneil Dalal\n"
		"Atharva Lath\n"
	},
	{ "hr",
		"Ivica Koli\n"
		"Zlatko Sehanović\n"
		"zvacet\n"
	},
	{ "hu",
		"Zsolt Bicskei\n"
		"Róbert Dancsó (dsjonny)\n"
		"Kálmán Kéménczy\n"
		"Zoltán Mizsei (miqlas)\n"
		"Bence Nagy\n"
		"Zoltán Szabó (Bird)\n"
	},
	{ "it",
		"Andrea Bernardi\n"
		"Dario Casalinuovo\n"
		"Francesco Franchina\n"
		"Lorenzo Frezza\n"
		"Mometto Nicola\n"
		"Michael Peppers\n"
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
		"Simonas Kazlauskas\n"
		"Rimas Kudelis\n"
	},
	{ "nl",
		"Floris Kint\n"
		"Meanwhile\n"
	},
	{ "pl",
		"Szymon Barczak\n"
		"Grzegorz Dąbrowski\n"
		"Hubert Hareńczyk\n"
		"Krzysztof Miemiec\n"
		"Artur Wyszyński\n"
	},
	{ "pt",
		"Marcos Alves (Xeon3D)\n"
		"Vasco Costa (gluon)\n"
		"Michael Vinícius de Oliveira (michaelvo)\n"
	},
	{ "pt_BR",
		"Cabral Bandeira (beyraq)\n"
		"Adriano A. Duarte (Sri_Dhryko)\n"
		"Tiago Matos (tiagoms)\n"
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
	},
	{ "ru",
		"Tatyana Fursic (iceid)\n"
		"StoroZ Gneva\n"
		"Rodastahm Islamov (RISC)\n"
		"Eugene Katashov (mrNoisy)\n"
		"Reznikov Sergei (Diver)\n"
		"Michael Smirnov\n"
		"Vladimir Vasilenko\n"
	},
	{ "sk",
		"Ivan Masár\n"
	},
	{ "sr",
		"Nikola Miljković\n"
	},
	{ "sv",
		"Patrik Gissberg\n"
		"Johan Holmberg\n"
		"Jimmy Olsson (phalax)\n"
		"Jonas Sundström\n"
		"Victor Widell\n"
	},
	{ "tr",
		"Hüseyin Aksu\n"
		"Halil İbrahim Azak\n"
		"Aras Ergus\n"
		"Enes Burhan Kuran\n"
		"Ali Rıza Nazlı\n"
	},
	{ "uk",
		"Mariya Pilipchuk\n"
		"Alex Rudyk (totish)\n"
		"Oleg Varunok\n"
	},
	{ "zh",
		"Pengfei Han (kurain)\n"
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
	"Fredrik Holmqvist\n" \
	"Philippe Houdoin\n" \
	"Ryan Leavengood\n" \
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
	"Brian Luft\n" \
	"Christof Lutteroth\n" \
	"Graham MacDonald\n" \
	"Jorge G. Mare (Koki)\n" \
	"Jan Matějek\n" \
	"Brian Matzon\n" \
	"Christopher ML Zumwalt May\n" \
	"Andrew McCall\n" \
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
