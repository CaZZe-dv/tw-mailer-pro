#include "UserVerificationLdap.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

namespace TwmailerPro {
UserVerificationLdap::UserVerificationLdap(){
    ldapUri = "ldap://ldap.technikum-wien.at:389";
    ldapVersion = LDAP_VERSION3;
    ldapSearchBaseDomainComponent = "dc=technikum-wien,dc=at";
    ldapHandle = NULL;
}

void UserVerificationLdap::setupLDAPConnection() {
    int rc = ldap_initialize(&ldapHandle, ldapUri);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_initialize failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Connected to LDAP server %s\n", ldapUri);
    
    rc = ldap_set_option(ldapHandle, LDAP_OPT_PROTOCOL_VERSION, &ldapVersion);
    if (rc != LDAP_OPT_SUCCESS) {
        fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        exit(EXIT_FAILURE);
    }
    
    rc = ldap_start_tls_s(ldapHandle, NULL, NULL);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        exit(EXIT_FAILURE);
    }
}



void UserVerificationLdap::bindLDAPCredentials(const char * username,const char *password) {    
    char ldapBindUser[256];
    char rawLdapUser[128];
    strcpy(rawLdapUser, username);
    sprintf(ldapBindUser, "uid=%s,ou=people,dc=technikum-wien,dc=at", rawLdapUser);
    printf("user set to: %s\n", ldapBindUser);

    char ldapBindPassword[256];
    strcpy(ldapBindPassword, password);

    BerValue bindCredentials;
    bindCredentials.bv_val = (char *)ldapBindPassword;
    bindCredentials.bv_len = strlen(ldapBindPassword);
    BerValue *servercredp;
    int rc = ldap_sasl_bind_s(ldapHandle, ldapBindUser, LDAP_SASL_SIMPLE, &bindCredentials, NULL, NULL, &servercredp);
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        exit(EXIT_FAILURE);
    }
    std::cout << "Authentification was successful" << std::endl;
}
}