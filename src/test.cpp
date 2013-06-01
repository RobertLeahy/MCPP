bool test () {

	RSAKey rsa_key;
	
	Vector<Byte> plaintext={1,2,3,4,5};
	
	Vector<Byte> encrypted=rsa_key.PrivateEncrypt(plaintext);
	
	StdOut << "Encrypted bytes:";
	for (Byte b : encrypted) StdOut << " 0x" << String(b,16);
	StdOut << Newline;
	
	Vector<Byte> recovered_plaintext=rsa_key.PublicDecrypt(encrypted);
	
	StdOut << Newline << "Recovered bytes:";
	for (Byte b : recovered_plaintext) StdOut << " 0x" << String(b,16);
	StdOut << Newline;
	
	StdOut << Newline << "ASN.1/DER representation of public key:" << Newline;
	bool first=true;
	for (Byte b : rsa_key.PublicKey()) {
	
		if (first) first=false;
		else StdOut << " ";
		
		//StdOut << "0x" << String(b,16);
		StdOut << b;
	
	}
	
	StdOut << Newline;
	
	//	Cancel main
	return true;

}
