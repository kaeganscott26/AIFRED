const __base = String(window.AIFRED_API_BASE_URL || window.location.origin).replace(/\/+$/, "");
const __apiV1Base = __base.endsWith("/v1") ? __base : `${__base}/v1`;
window.AIFRED_CONFIG = {
  apiBase: __base,
  apiV1Base: __apiV1Base,
  contactEmail: "north3rnlight3rofficial@outlook.com",
  paypal: {},
  downloadUrls: {
    releaseNotes: `${__base}/assets/docs/aifred-release-notes.txt`
  },
  productPrice: "$5 one-time beta access"
};
