(function ($) {
  Drupal.behaviors.lesson = {
    attach: function (context, settings) {
      // Passing these into the init method doesn't
      // seem to work.
      $.deck.defaults.keys = {
        next: [],
        previous: []
      };

      $.deck('section', {
        hashPrefix: '',
      });
      $.deck('enableScale');

      var manager, transcript;
      $('audio').bind('loadeddata', function () {
        var len = $(this).attr('duration');
        manager = new SegmentManager(len, settings.lesson_progress, $(this));
        transcript = new Transcript($('.transcript').text(), len);
      });

      $('audio').bind('seeking', function () {
        transcript.update($(this).attr('currentTime'));
      });

      $('audio').bind('timeupdate', function (e) {
        var t = $(this).attr('currentTime');

        manager.updateSegment(t);
        transcript.update($(this).attr('currentTime'));
      });

      var lastProgress = null;

      setInterval(function () {
        var progress = manager.toString();

        if (progress !== lastProgress) {
          $.post('/node/' + settings.nid + '/audio_progress', {progress: progress});
          lastProgress = progress;
        }
      }, 5000);
    }
  }
})(jQuery);

var Transcript = function (text, maxlen) {
  this.times = {};
  this.end = maxlen;

  var text = text.split('\n');
  for (var i = 0; i < text.length; i++) {
    var t = text[i].substring(0, text[i].indexOf(' ')).split(':');
    t = parseInt(t[0]) * 60 + parseInt(t[1]);

    this.times[t] = text[i].substr(text[i].indexOf(' ') + 1);
  }

  this.times[0] = '[1]';
};

Transcript.prototype.update = function (timestamp) {
  timestamp = parseInt(timestamp);
  var t, slide;
  for (var i in this.times) {
    if (i <= timestamp) {
      t = this.times[i];
      if (t[0] == '[') {
        var num = t.substring(t.indexOf('[')+1, t.indexOf(']'));
        slide = num-1;
      }
    } else {
      break;
    }
  }

  jQuery.deck('go', slide);
}

var SEGMENTS = 50;

var SegmentManager = function (length, progress, $obj) {
  this.segments = {};
  length = Math.floor(length);

  var temp = parseInt(progress, 16).toString(2);

  var segmentLength = length / SEGMENTS;
  this.currentSegment = 0;
  var somethingComplete = false;

  for (var i = 0; i < SEGMENTS; i++) {
    var l = Math.round(segmentLength * i);
    var next = Math.round(segmentLength * (i+1));
    var max = (next < length) ? next : Math.floor(length);
    this.segments[l] = new Segment(l, max);

    if (temp[i] == 1) {
      this.segments[l].complete = true;
      somethingComplete = true;
    } else if (this.currentSegment == 0 && somethingComplete) {
      this.currentSegment = l;
    }
  }

  // If we're still at 0 but something has been listened to, then
  // that means everything has been listened to.
  if (this.currentSegment == 0 && somethingComplete) {
    this.currentSegment = Math.round(segmentLength * (SEGMENTS-1));
  }

  $obj.attr('currentTime', this.segments[this.currentSegment].min);
};

SegmentManager.prototype.getSegments = function () {
  return this.segments;
};

SegmentManager.prototype.updateSegment = function (t) {
  if (t >= this.segments[this.currentSegment].max || t <= this.segments[this.currentSegment].min) {
    for (var i in this.segments) {
      if (t < this.segments[i].max && t > this.segments[i].min) {
        this.currentSegment = i;
        break;
      }
    }
  }

  this.segments[this.currentSegment].track(t);
};

SegmentManager.prototype.toString = function () {
  var binary = '';
  for (var i in this.segments) {
    binary = ((this.segments[i].complete) ? 1 : 0) + binary;
  }

  return parseInt(binary, 2).toString(16);
};

var Segment = function (start, end) {
  this.min = start;
  this.max = end;

  this.localMin = 1e100;
  this.localMax = -1;

  this.complete = false;
};

Segment.prototype.track = function (time) {
  if (this.complete) {
    return;
  }

  time = Math.round(time);

  if (time > this.localMax) {
    this.localMax = time;
  } else if (time < this.localMin) {
    this.localMin = time;
  }

  if (this.localMin <= this.min && this.localMax >= this.max) {
    this.complete = true;
  }
};