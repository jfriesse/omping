![Open Multicast Ping banner](extras/img/omping-banner.png)

### Overview
Omping (Open Multicast Ping) is tool to test IP multicast functionality primarily in local network.

### Features
 * Similar user experience as classic ping tool
 * Ping multiple hosts at once
 * Implementation of RFC Draft [http://tools.ietf.org/html/draft-ietf-mboned-ssmping-08]
 * Any-source and Source-specific Multicast

### Installation
#### Fedora
Fedora contain omping package. Use yum for installation.
```
$ yum install omping
```

#### Source code
For stable version, use https://github.com/jfriesse/omping/releases/download/0.0.4/omping-0.0.4.tar.gz. For newest git, use
```
$ git clone git://github.com/jfriesse/omping.git
$ cd omping
$ make
```

#### Support
Please use GitHub issues.

#### Defects
If a defect is found in omping, please create new issue. You can also try to find and fix bug and create pull
request.

#### Who
This project is maintained by [Jan Friesse](mailto:jfriesseATredhatDOTcom) ([Red Hat](http://www.redhat.com))
