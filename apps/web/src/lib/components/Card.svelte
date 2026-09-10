<script lang="ts">
  export let title: string = '';
  export let description: string = '';
</script>

<div class="rounded-xl border-2 border-primary/20 bg-card text-card-foreground shadow-[0_8px_30px_rgb(0,0,0,0.12)] {$$props.class || ''} transition-all hover:shadow-[0_8px_30px_rgb(0,0,0,0.2)] hover:border-primary/40">
  {#if title || description}
    <div class="flex flex-col space-y-1.5 p-6 pb-4">
      {#if title}
        <h3 class="font-bold text-xl leading-none tracking-tight text-primary">{title}</h3>
      {/if}
      {#if description}
        <p class="text-sm text-muted-foreground font-medium">{description}</p>
      {/if}
    </div>
  {/if}
  <div class="p-6 pt-0">
    <slot />
  </div>
</div>
