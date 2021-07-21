/*
MIT License

Copyright (c) 2020 Haiku, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

;(function (window, doc) {

	const bodyClassList = doc.getElementsByTagName('body')[0].classList;
	const contentDiv = doc.getElementById('content');
	const passive = { passive: true };

	let scrollTimeout = null;
	let internalHashChange = false;
	let currentTopic = null;

	function slides() {
		return bodyClassList.contains('slides');
	}

	function toggleFormat() {
		processPendingScroll();
		bodyClassList.toggle('slides');
		topics.get(currentTopic).slide.scrollIntoView(true);
		setLocationHash('');
	}

	function nextTopic() {
		processPendingScroll();
		const topic = topics.get(currentTopic);
		if (topic != null && topic.next != null) {
			setTopic(topic.next);
			topics.get(topic.next).slide.scrollIntoView(true);
		}
	}

	function previousTopic() {
		processPendingScroll();
		const topic = topics.get(currentTopic);
		if (topic != null && topic.prev != null) {
			setTopic(topic.prev);
			topics.get(topic.prev).slide.scrollIntoView(true);
		}
	}

	/* The translation tool wants exactly <div id="content">\n<div>
	 * to insert its "translation incomplete" box. Removing the topic
	 * class from the welcome page breaks the slide layout. Adding it
	 * back on load only shows the box in the first slide. Having an
	 * extra div when there's no box adds too much whitespace to the
	 * top of the slides. So we remove it here if it has no content.
	 */
	{
		let emptyDiv = doc.querySelector('#content>div:empty');
		if (emptyDiv != null) {
			emptyDiv.remove();
		}
	}

	let topics = new Map();
	{
		let prev = null;
		for (let topicDiv of doc.querySelectorAll('div.topic')) {
			let topic = '#' + topicDiv.querySelector('h1 a').name;
			topics.set(topic, {
				slide: topicDiv,
				navdot: null,
				next: null,
				prev: prev,
			});
			prev = topic;
		}
		for (let dot of doc.querySelectorAll('.navdots a acronym')) {
			topics.get((new URL(dot.parentNode.href)).hash).navdot = dot;
		}
		for (let [k,v] of topics) {
			prev = v.prev;
			if (prev !== null) {
				topics.get(prev).next = k;
			}
		}
	}

	function setTopic(hash) {
		if (hash == null || hash.length == 0) {
			hash = topics.keys().next().value;
		} else if (!topics.has(hash)) {
			hash = hash.substring(1);
			let element = doc.getElementById(hash)
							|| doc.querySelector('[name='+hash+']');
			if (element != null) {
				element = element.closest('div.topic');
			}
			if (element != null) {
				hash = '#' + element.querySelector('h1 a').name;
			}
			if (!topics.has(hash)) {
				hash = topics.keys().next().value;
			}
		}

		if (hash === currentTopic) return;

		let topic;
		if (currentTopic !== null) {
			topic = topics.get(currentTopic);
			topic.slide.classList.remove('current-page');
			topic.navdot.classList.remove('current-page');
		}
		topic = topics.get(hash);
		topic.slide.classList.add('current-page');
		topic.navdot.classList.add('current-page');
		currentTopic = hash;
	}


	function checkPosition() {
		const containerRect = contentDiv.getBoundingClientRect();
		const minTop = containerRect.top;
		const maxTop = minTop + (containerRect.bottom - minTop) / 3;
		let bestValue = -9999;
		let bestTopic = null;
		for (let [topic, v] of topics) {
			const topicRect = v.slide.getBoundingClientRect();
			if (topicRect.top < maxTop) {
				if (topicRect.top >= minTop) {
					if (topicRect.top < bestValue || bestValue < minTop) {
						bestValue = topicRect.top;
						bestTopic = topic;
					}
				} else if (topicRect.top > bestValue) {
					bestValue = topicRect.top;
					bestTopic = topic;
				}
			}
		}
		if (bestTopic !== null) {
			setTopic(bestTopic);
		}
	}

	function checkScroll() {
		scrollTimeout = null;
		if (!slides()) {
			checkPosition();
		}
	}

	function processPendingScroll() {
		if (scrollTimeout != null) {
			window.clearTimeout(scrollTimeout);
			checkScroll();
		}
	}

	function positionChangeHandler() {
		if (scrollTimeout === null) {
			scrollTimeout = window.setTimeout(checkScroll, 200);
		}
	}

	contentDiv.addEventListener('scroll', positionChangeHandler, passive);
	window.addEventListener('resize', positionChangeHandler, passive);
	window.addEventListener('orientationchange', positionChangeHandler, passive);


	function setLocationHash(hash) {
		if (window.location.hash != hash) {
			internalHashChange = true;
			window.location.hash = hash;
		}
	}

	window.addEventListener('hashchange', function() {
		if (!internalHashChange) {
			setTopic(window.location.hash);
		}
		internalHashChange = false;
	}, passive);

	doc.getElementById('toggle').addEventListener(
		'click', toggleFormat, passive);
	doc.getElementById('prevtopic').addEventListener(
		'click', previousTopic, passive);
	doc.getElementById('nexttopic').addEventListener(
		'click', nextTopic, passive);
	doc.getElementById('prevtopic-bottom').addEventListener(
		'click', previousTopic, passive);
	doc.getElementById('nexttopic-bottom').addEventListener(
		'click', nextTopic, passive);

	for (let element of doc.querySelectorAll('.hide-no-js')) {
		element.classList.remove('hide-no-js');
	}

	/* Body is fixed, with the scrollable element being #content.
	 * So let's scroll #content with events triggered outside.
	 * No fancy inertia or smooth scrolling, though.
	 */
	{
		const targetInContent = (e) => e.target.closest('#content') != null;
		const lines = (l) => 20 * l;
		const pages = (p) => p * (contentDiv.getBoundingClientRect().height - lines(1));

		let wantScroll = 0;
		let lockedScroll = false;

		function lockScroll() {
			lockedScroll = true;
			window.setTimeout(function () {
				lockedScroll = false;
			}, 400);
		}

		function doScroll() {
			if (lockedScroll) {
				wantScroll = 0;
				return;
			}

			let curScroll = contentDiv.scrollTop;
			contentDiv.scrollTop += wantScroll;
			if (curScroll == contentDiv.scrollTop) {
				wantScroll /= lines(3);
				if (wantScroll > 1) {
					nextTopic();
					lockScroll();
				} else if (wantScroll < -1) {
					previousTopic();
					lockScroll();
				}
			}
			wantScroll = 0;
		}

		function requestScroll(px) {
			if (wantScroll === 0) {
				window.requestAnimationFrame(doScroll);
			}
			wantScroll += px;
		}

		function forceScroll(px) {
			lockedScroll = false;
			requestScroll(px);
		}

		doc.addEventListener('wheel', function (event) {
			if (event.shiftKey || event.ctrlKey || event.altKey || event.metaKey)
				return;
			if (event.target.closest('.lang-menu') != null)
				return;
			switch (event.deltaMode) {
			case event.DOM_DELTA_PIXEL:
				requestScroll(event.deltaY);
				break;
			case event.DOM_DELTA_PAGE:
				requestScroll(pages(event.deltaY > 0 ? 1 : -1));
				break;
			default:
				requestScroll(lines(event.deltaY));
				break;
			}
		}, passive);
		doc.addEventListener('keydown', function (event) {
			if (!targetInContent(event)) {
				switch (event.key) {
				case 'PageDown':
					forceScroll(pages(1));
					break;
				case 'PageUp':
					forceScroll(-pages(1));
					break;
				case 'ArrowDown':
					requestScroll(lines(1));
					break;
				case 'ArrowUp':
					requestScroll(lines(-1));
					break;
				}
			}
			if (event.ctrlKey || event.metaKey) {
				switch (event.key) {
				case 'ArrowLeft':
					previousTopic();
					break;
				case 'ArrowRight':
					nextTopic();
					break;
				case 'm':
					toggleFormat();
					break;
				}
			}
		}, passive);
	}

	setTopic(window.location.hash);

	// Change to slide mode, except in the translation tool
	if (typeof source_strings === "undefined") {
		toggleFormat();
	}

} (window, document));
