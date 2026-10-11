



fn Midi::Process(time, root)
{
	static Poll::Struct ~ pd;
	put (pd + Poll::Struct::FD    ).0 = IO::STD::IN;
	put (pd + Poll::Struct::EVENTS).0 = Poll::Event::POLLIN;

	// a midi packet is only 3 bytes.
	// usually the first byte specifies the
	// command / channel.
	static 1 ~ bpacket;
	static 3 ~ qpacket;

	lab loop;
		jump done ~ syscall(SYSCALL::POLL,
			pd, 1, 0
		) < 1;

		jump done ~ syscall(SYSCALL::READ,
			IO::STD::IN,
			bpacket,
			3,
		) < 1;
		Mem::FromBytes(qpacket, bpacket, 3);

		put func   = qpacket.0;
		put number = qpacket.1;

		put action = func >> 4;
		put chan   = func & 15;

		jump note_on  ~ action == 9;
		jump note_off ~ action == 8;

		// unknown command, skip
		jump loop;

	lab note_on;
		put vel = qpacket.2;
		jump note_off ~ vel == 0;

		put note = Note::New(time, number);
		Note::Link(root, note);
		jump loop;

	lab note_off;
		put note = Note::Find(root, number);
		put note.Note::RELEASED = Bool::TRUE;
		jump loop;

lab done;
}





