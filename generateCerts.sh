#!/bin/bash

# Set variables
CERT_FILE="server.crt"
KEY_FILE="server.key"
DAYS=365 # Validity period of the certificate in days
SUBJECT="/C=US/ST=State/L=City/O=Organization/OU=Organizational Unit/CN=localhost"
# Generate the private key
openssl genrsa -out "$KEY_FILE" 2048

# Generate the certificate signing request (CSR)
openssl req -new -key "$KEY_FILE" -out "$CERT_FILE.csr" -subj "$SUBJECT"

# Generate the self-signed certificate
openssl x509 -req -days "$DAYS" -in "$CERT_FILE.csr" -signkey "$KEY_FILE" -out "$CERT_FILE"

# Remove the CSR file
rm "$CERT_FILE.csr"

echo "Self-signed certificate ($CERT_FILE) and private key ($KEY_FILE) generated successfully."
