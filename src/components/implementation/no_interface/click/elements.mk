# Generated by 'click-buildtool elem2make' on Thu Jun 29 15:23:58 EDT 2017
CLICK_ELEMENTS_OBJ_DIR__0=$(CLICK_ROOT)/elements/analysis
CLICK_ELEMENTS_OBJ_DIR__1=$(CLICK_ROOT)/elements/app
CLICK_ELEMENTS_OBJ_DIR__2=$(CLICK_ROOT)/elements/aqm
CLICK_ELEMENTS_OBJ_DIR__3=$(CLICK_ROOT)/elements/ethernet
CLICK_ELEMENTS_OBJ_DIR__4=$(CLICK_ROOT)/elements/icmp
CLICK_ELEMENTS_OBJ_DIR__5=$(CLICK_ROOT)/elements/ip
CLICK_ELEMENTS_OBJ_DIR__6=$(CLICK_ROOT)/elements/minios
CLICK_ELEMENTS_OBJ_DIR__6_1=$(CLICK_ROOT)/elements/cos
CLICK_ELEMENTS_OBJ_DIR__7=$(CLICK_ROOT)/elements/simple
CLICK_ELEMENTS_OBJ_DIR__8=$(CLICK_ROOT)/elements/standard
CLICK_ELEMENTS_OBJ_DIR__9=$(CLICK_ROOT)/elements/tcpudp
CLICK_ELEMENTS_OBJ_DIR__10=$(CLICK_ROOT)/elements/test
CLICK_ELEMENTS_OBJ_DIR__11=$(CLICK_ROOT)/elements/threads

CLICK_ELEMENTS_OBJS0__0 = \
$(CLICK_ELEMENTS_OBJ_DIR__0)/adjusttimestamp.o \
$(CLICK_ELEMENTS_OBJ_DIR__0)/aggregateipflows.o \
$(CLICK_ELEMENTS_OBJ_DIR__0)/aggregatenotifier.o \
$(CLICK_ELEMENTS_OBJ_DIR__0)/anonipaddr.o \
$(CLICK_ELEMENTS_OBJ_DIR__0)/eraseippayload.o \
$(CLICK_ELEMENTS_OBJ_DIR__0)/settimestampdelta.o \
$(CLICK_ELEMENTS_OBJ_DIR__0)/storetimestamp.o \
$(CLICK_ELEMENTS_OBJ_DIR__0)/storeudptimeseqrecord.o \
$(CLICK_ELEMENTS_OBJ_DIR__0)/timefilter.o \
$(CLICK_ELEMENTS_OBJ_DIR__0)/timerange.o \
$(CLICK_ELEMENTS_OBJ_DIR__0)/timesortedsched.o \

CLICK_ELEMENTS_OBJS0__1 = \
$(CLICK_ELEMENTS_OBJ_DIR__1)/ftpportmapper.o \

CLICK_ELEMENTS_OBJS0__2 = \
$(CLICK_ELEMENTS_OBJ_DIR__2)/adaptivered.o \
$(CLICK_ELEMENTS_OBJ_DIR__2)/red.o \

CLICK_ELEMENTS_OBJS0__3 = \
$(CLICK_ELEMENTS_OBJ_DIR__3)/arpfaker.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/arpprint.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/arpquerier.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/arpresponder.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/arptable.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/checkarpheader.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/ensureether.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/etherencap.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/ethermirror.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/etherpausesource.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/ethervlanencap.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/getetheraddress.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/hostetherfilter.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/setetheraddress.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/setvlananno.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/storeetheraddress.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/stripethervlanheader.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/vlandecap.o \
$(CLICK_ELEMENTS_OBJ_DIR__3)/vlanencap.o \

CLICK_ELEMENTS_OBJS0__4 = \
$(CLICK_ELEMENTS_OBJ_DIR__4)/checkicmpheader.o \
$(CLICK_ELEMENTS_OBJ_DIR__4)/icmperror.o \
$(CLICK_ELEMENTS_OBJ_DIR__4)/icmpipencap.o \
$(CLICK_ELEMENTS_OBJ_DIR__4)/icmppingencap.o \
$(CLICK_ELEMENTS_OBJ_DIR__4)/icmppingresponder.o \
$(CLICK_ELEMENTS_OBJ_DIR__4)/icmppingrewriter.o \
$(CLICK_ELEMENTS_OBJ_DIR__4)/icmprewriter.o \
$(CLICK_ELEMENTS_OBJ_DIR__4)/icmpsendpings.o \

CLICK_ELEMENTS_OBJS0__5 = \
$(CLICK_ELEMENTS_OBJ_DIR__5)/checkipheader.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/checkipheader2.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/decipttl.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/fixipsrc.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/getipaddress.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipaddrpairrewriter.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipaddrrewriter.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipclassifier.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipencap.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipfieldinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipfilter.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipfragmenter.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipgwoptions.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipinputcombo.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipmirror.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipnameinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipoutputcombo.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipprint.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ipreassembler.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/iprewriterbase.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/iproutetable.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/iprwmapping.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/iprwpattern.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/iprwpatterns.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/lineariplookup.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/lookupiproute.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/markipce.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/markipheader.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/radixiplookup.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/rfc2507c.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/rfc2507d.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/ripsend.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/rripmapper.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/setipaddress.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/setipchecksum.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/setipdscp.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/setipecn.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/setrandipaddress.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/siphmapper.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/sortediplookup.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/storeipaddress.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/stripipheader.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/truncateippayload.o \
$(CLICK_ELEMENTS_OBJ_DIR__5)/unstripipheader.o \

CLICK_ELEMENTS_OBJS0__6 = \
$(CLICK_ELEMENTS_OBJ_DIR__6)/fromdevice.o \
$(CLICK_ELEMENTS_OBJ_DIR__6)/todevice.o \

CLICK_ELEMENTS_OBJS0__6_1 = \
$(CLICK_ELEMENTS_OBJ_DIR__6_1)/toshmem.o \
$(CLICK_ELEMENTS_OBJ_DIR__6_1)/fromshmem.o \

CLICK_ELEMENTS_OBJS0__7 = \
$(CLICK_ELEMENTS_OBJ_DIR__7)/simpleidle.o \
$(CLICK_ELEMENTS_OBJ_DIR__7)/simplepriosched.o \
$(CLICK_ELEMENTS_OBJ_DIR__7)/simplepullswitch.o \
$(CLICK_ELEMENTS_OBJ_DIR__7)/simplerrsched.o \

CLICK_ELEMENTS_OBJS0__8 = \
$(CLICK_ELEMENTS_OBJ_DIR__8)/addressinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/align.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/alignmentinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/annotationinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/averagecounter.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/bandwidthmeter.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/bandwidthshaper.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/block.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/burster.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/bwratedsplitter.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/bwratedunqueue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/bypass.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/checkcrc32.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/checklength.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/checkpaint.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/classification.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/classifier.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/clickyinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/clipboard.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/compblock.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/counter.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/delayshaper.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/delayunqueue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/devirtualizeinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/discard.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/discardnofree.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/dropbroadcasts.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/drr.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/errorelement.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/flowinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/frontdropqueue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/fullnotequeue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/hashswitch.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/hub.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/idle.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/infinitesource.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/inputswitch.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/linkunqueue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/markmacheader.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/messageelement.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/meter.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/mixedqueue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/notifierqueue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/nullelement.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/nulls.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/paint.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/paintswitch.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/painttee.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/portinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/print.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/priosched.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/pullswitch.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/quicknotequeue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/quitwatcher.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/randomerror.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/randomsample.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/randomsource.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/randomswitch.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/ratedsource.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/ratedsplitter.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/ratedunqueue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/resize.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/rrsched.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/rrswitch.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/scheduleinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/setannobyte.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/setcrc32.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/setpackettype.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/settimestamp.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/shaper.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/simplequeue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/staticpullswitch.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/staticswitch.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/storedata.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/stridesched.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/strideswitch.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/strip.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/striptonet.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/suppressor.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/switch.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/tee.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/threadsafequeue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/timedsink.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/timedsource.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/timedunqueue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/truncate.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/unqueue.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/unqueue2.o \
$(CLICK_ELEMENTS_OBJ_DIR__8)/unstrip.o \

CLICK_ELEMENTS_OBJS0__9 = \
$(CLICK_ELEMENTS_OBJ_DIR__9)/checktcpheader.o \
$(CLICK_ELEMENTS_OBJ_DIR__9)/checkudpheader.o \
$(CLICK_ELEMENTS_OBJ_DIR__9)/dynudpipencap.o \
$(CLICK_ELEMENTS_OBJ_DIR__9)/iprewriter.o \
$(CLICK_ELEMENTS_OBJ_DIR__9)/settcpchecksum.o \
$(CLICK_ELEMENTS_OBJ_DIR__9)/setudpchecksum.o \
$(CLICK_ELEMENTS_OBJ_DIR__9)/tcpfragmenter.o \
$(CLICK_ELEMENTS_OBJ_DIR__9)/tcpipsend.o \
$(CLICK_ELEMENTS_OBJ_DIR__9)/tcprewriter.o \
$(CLICK_ELEMENTS_OBJ_DIR__9)/udpipencap.o \
$(CLICK_ELEMENTS_OBJ_DIR__9)/udprewriter.o \

CLICK_ELEMENTS_OBJS0__10 = \
$(CLICK_ELEMENTS_OBJ_DIR__10)/bhmtest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/bitvectortest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/checkpacket.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/comparepackets.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/confparsetest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/cryptotest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/dequetest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/errortest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/functiontest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/handlertask.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/hashtabletest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/heaptest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/listtest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/neighborhoodtest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/notifierdebug.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/notifiertest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/nulltask.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/packettest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/queueyanktest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/randomseed.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/schedordertest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/sorttest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/timertest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/tokenbuckettest.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/upstreamnotifier.o \
$(CLICK_ELEMENTS_OBJ_DIR__10)/vectortest.o \

CLICK_ELEMENTS_OBJS0__11 = \
$(CLICK_ELEMENTS_OBJ_DIR__11)/spinlockacquire.o \
$(CLICK_ELEMENTS_OBJ_DIR__11)/spinlockinfo.o \
$(CLICK_ELEMENTS_OBJ_DIR__11)/spinlockrelease.o \
$(CLICK_ELEMENTS_OBJ_DIR__11)/staticthreadsched.o \

CLICK_ELEMENTS_OBJS0 = \
$(CLICK_ELEMENTS_OBJS0__0) \
$(CLICK_ELEMENTS_OBJS0__1) \
$(CLICK_ELEMENTS_OBJS0__2) \
$(CLICK_ELEMENTS_OBJS0__3) \
$(CLICK_ELEMENTS_OBJS0__4) \
$(CLICK_ELEMENTS_OBJS0__5) \
$(CLICK_ELEMENTS_OBJS0__6_1) \
$(CLICK_ELEMENTS_OBJS0__7) \
$(CLICK_ELEMENTS_OBJS0__8) \
$(CLICK_ELEMENTS_OBJS0__9) \
$(CLICK_ELEMENTS_OBJS0__10) \
$(CLICK_ELEMENTS_OBJS0__11) \

