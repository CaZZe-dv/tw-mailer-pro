#pragma once

#include <ldap.h>

namespace TwmailerPro {
class UserVerificationLdap {
private:
    const char *ldapUri;
    int ldapVersion;
    const char *ldapSearchBaseDomainComponent;
    LDAP *ldapHandle;

public:
    UserVerificationLdap();
    void setupLDAPConnection();
    void bindLDAPCredentials(const char *ldapBindUser, const char *ldapBindPassword);
};
}

