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
    std::string bindLDAPCredentials(const char *ldapBindUser, const char *ldapBindPassword);
};
}

