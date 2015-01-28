#include "Server.h"


int main() { 
	Server s(8384, 8);
	Session::server = &s;
	s.Start();
} 